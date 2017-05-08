#include "txn.h"
#include "row.h"
#include "row_mocc_silo.h"
#include "mem_alloc.h"
#include "manager.h"

#if CC_ALG==MOCC_SILO

void Row_mocc_silo::init(row_t * row)
{
	_row = row;
#if ATOMIC_WORD
	_tid_word = 0;
#else
	_latch = (pthread_mutex_t *) _mm_malloc(sizeof(pthread_mutex_t), 64);
	pthread_mutex_init( _latch, NULL );
	_tid = 0;
#endif

#if RECORD_TEMP_STATS
	#if STAT_TYPE == PER_RECORD_TEMPS
		temp = 0;
	#elif STAT_TYPE == GLOBAL_HASHTABLE_TEMPS
		glob_manager->add_temp_stat((uint64_t)_row);
	#else
		throw std::invalid_argument("Specified stat type is not implemented");
	#endif

	#if HOT_LOCK_RECORDS
		owners = NULL;
		waiters_head = NULL;
		waiters_tail = NULL;
		owner_cnt = 0;
		waiter_cnt = 0;
		lock_type = LOCK_NONE;
	#endif

#endif
}

RC Row_mocc_silo::access(txn_man * txn, TsType type, row_t * local_row) {
#if ATOMIC_WORD
	uint64_t v = 0;
	uint64_t v2 = 1;
	while (v2 != v) {
		v = _tid_word;
		while (v & LOCK_BIT) {
			PAUSE
			v = _tid_word;
		}
		local_row->copy(_row);
		COMPILER_BARRIER
		v2 = _tid_word;
	}
	txn->last_tid = v & (~LOCK_BIT);
#else
	lock();
	local_row->copy(_row);
	txn->last_tid = _tid;
	release();
#endif
	return RCOK;
}

void Row_mocc_silo::assert_lock() {
#if ATOMIC_WORD
	assert(_tid_word & LOCK_BIT);
#else
#endif
}

bool Row_mocc_silo::validate(txn_man * txn, ts_t tid, bool in_write_set) {
#if ATOMIC_WORD
	uint64_t v = _tid_word;
	if (in_write_set)
		return tid == (v & (~LOCK_BIT));

	if (v & LOCK_BIT)
		return false;
	else if (tid != (v & (~LOCK_BIT)))
		return false;
	else
		return true;
#else
	if (in_write_set)
		return tid == _tid;
	if (!try_lock())
		return false;
	bool valid = (tid == _tid);
	release();
	return valid;
#endif
}

// write from this copy record to the original one
void Row_mocc_silo::write(row_t * data, uint64_t tid) {
	_row->copy(data);
#if ATOMIC_WORD
	uint64_t v = _tid_word;
	M_ASSERT(tid > (v & (~LOCK_BIT)) && (v & LOCK_BIT), "tid=%ld, v & LOCK_BIT=%ld, v & (~LOCK_BIT)=%ld\n", tid, (v & LOCK_BIT), (v & (~LOCK_BIT)));
	_tid_word = (tid | LOCK_BIT);
#else
	_tid = tid;
#endif
}

// Lock the record
void Row_mocc_silo::lock() {
#if ATOMIC_WORD
	uint64_t v = _tid_word;
	while ((v & LOCK_BIT) || !__sync_bool_compare_and_swap(&_tid_word, v, v | LOCK_BIT)) {
		PAUSE
		v = _tid_word;
	}
#else
	pthread_mutex_lock( _latch );
#endif
}

// releases currently held lock
void Row_mocc_silo::release() {
#if ATOMIC_WORD
	assert(_tid_word & LOCK_BIT);
	_tid_word = _tid_word & (~LOCK_BIT);
#else
	pthread_mutex_unlock( _latch );
#endif
}

bool Row_mocc_silo::try_lock() {
#if ATOMIC_WORD
	uint64_t v = _tid_word;
	if (v & LOCK_BIT) // already locked
		return false;
	return __sync_bool_compare_and_swap(&_tid_word, v, (v | LOCK_BIT));
#else
	return pthread_mutex_trylock( _latch ) != EBUSY;
#endif
}

uint64_t Row_mocc_silo::get_tid()
{
#if ATOMIC_WORD
	assert(ATOMIC_WORD);
	return _tid_word & (~LOCK_BIT);
#else
	return _tid;
#endif
}

////////////////////////////
// TEMPERATURE STATISTICS
////////////////////////////

#if RECORD_TEMP_STATS
	//Will get the temperature of the record
	unsigned short Row_mocc_silo::get_temp()
	{
	#if STAT_TYPE == PER_RECORD_TEMPS
		return temp;
	#elif STAT_TYPE == GLOBAL_HASHTABLE_TEMPS
		return glob_manager->get_temp((uint64_t)_row);
	#else
		throw std::invalid_argument("Specified stat type is not implemented");
	#endif
	}

	void Row_mocc_silo::update_temp_stat()
	{
	#if STAT_TYPE == PER_RECORD_TEMPS
		temp++;
	#elif STAT_TYPE == GLOBAL_HASHTABLE_TEMPS
		return glob_manager->update_temp_stat((uint64_t)_row);
	#else
		throw std::invalid_argument("Specified stat type is not implemented");
	#endif
	}
#endif

/////////////////////////
// HOT LOCKING
/////////////////////////

#if RECORD_TEMP_STATS && HOT_LOCK_RECORDS
	RC Row_mocc_silo::hot_access(txn_man * txn, lock_t type, row_t * local_row) {
		uint64_t *txnids = NULL;
		int txncnt = 0;
		return hot_access(txn, type, txnids, txncnt, local_row);
	}

	RC Row_mocc_silo::hot_access(txn_man * txn, lock_t type, uint64_t* &txnids, int &txncnt, row_t * local_row) {
		RC rc;
		lock();
		// for sanity
		assert(owner_cnt <= g_thread_cnt);
		assert(waiter_cnt < g_thread_cnt);
		// Also add debug stuff from row_lock

		bool conflict = conflict_lock(lock_type, type);
		if (LOCK_STRATEGY == NO_WAIT_HOT_LOCK && !conflict) {
			if (waiters_head && txn->get_ts() < waiters_head->txn->get_ts())
				conflict = true;
		}
		if (conflict) {
		#if LOCK_STRATEGY == NO_WAIT_HOT_LOCK
			rc = Abort;
		#elif LOCK_STRATEGY == WAIT_DIE_HOT_LOCK
			/// Note: To avoid starvation, a process should not be assigned
       		/// a new timestamp each time it restarts.
			bool canwait = true;
			MoccLockEntry * en = owners;
			while (en != NULL) {
				// Run through all the lock entries and check
				// whether any of the txns currently holding or
				// waiting for the lock are older (have a lower ts)
				// Abort if any are older
                if (en->txn->get_ts() < txn->get_ts()) {
					canwait = false;
					break;
				}
				en = en->next;
			}
			if (canwait) {
				// insert txn to the right position
				// the waiter list is always in timestamp order
				MoccLockEntry * entry = get_entry();
				entry->txn = txn;
				entry->type = type;
				en = waiters_head;
				while (en != NULL && txn->get_ts() < en->txn->get_ts())
					en = en->next;
				if (en) {
					LIST_INSERT_BEFORE(en, entry);
					if (en == waiters_head)
						waiters_head = entry;
				} else
					LIST_PUT_TAIL(waiters_head, waiters_tail, entry);
				waiter_cnt ++;
                txn->lock_ready = false;

				// Wait until lock is ready
				while (!txn->lock_ready) {
					continue;  // should I pause?
				}
				rc = RCOK;
            } else {
				// txn can't wait
                rc = Abort;
			}
		#else
			throw std::invalid_argument("Specified stat type is not implemented");
		#endif

		} else {
			// No conflict, so insert entry into owners
			MoccLockEntry * entry = get_entry();
			entry->type = type;
			entry->txn = txn;
			STACK_PUSH(owners, entry);
			owner_cnt ++;
			lock_type = type;
			rc = RCOK;
		}

		// Have the lock, so do occ stuff (make a copy) and then unlatch
		if (rc == RCOK) {
			local_row->copy(_row);
			txn->last_tid = _tid;
		}
		release();
		return rc;
	}


	RC Row_mocc_silo::hot_release(txn_man * txn) {
		// Assumes that the lock has already been taken

		if (owner_cnt == 0 && owners == NULL) return RCOK; // Not currently hotlocked

		// Try to find the entry in the owners
		MoccLockEntry * en = owners;
		MoccLockEntry * prev = NULL;

		while (en != NULL && en->txn != txn) {
			prev = en;
			en = en->next;
		}
		if (en) {
			// Found this txn's entry in owner list
			// then remove this txn's entry from the owners list
			if (prev) prev->next = en->next;
			else owners = en->next;
			return_entry(en);
			owner_cnt --;
			if (owner_cnt == 0)
				lock_type = LOCK_NONE;
		} else {
			// Not in owners list, try waiters list.
			en = waiters_head;
			while (en != NULL && en->txn != txn)
				en = en->next;
			ASSERT(en);
			LIST_REMOVE(en);
			if (en == waiters_head)
				waiters_head = en->next;
			if (en == waiters_tail)
				waiters_tail = en->prev;
			return_entry(en);
			waiter_cnt --;
		}

		if (owner_cnt == 0) ASSERT(lock_type == LOCK_NONE);

		MoccLockEntry * entry;
		// If any waiter can join the owners, just do it!
		while (waiters_head && !conflict_lock(lock_type, waiters_head->type)) {
			LIST_GET_HEAD(waiters_head, waiters_tail, entry);
			STACK_PUSH(owners, entry);
			owner_cnt ++;
			waiter_cnt --;
			ASSERT(entry->txn->lock_ready == false);
			entry->txn->lock_ready = true;
			lock_type = entry->type;
		}
		return FINISH;
	}

	bool Row_mocc_silo::conflict_lock(lock_t l1, lock_t l2) {
		if (l1 == LOCK_NONE || l2 == LOCK_NONE)
			return false;
		else if (l1 == LOCK_EX || l2 == LOCK_EX)
			return true;
		else
			return false;
	}

	MoccLockEntry * Row_mocc_silo::get_entry() {
		MoccLockEntry * entry = (MoccLockEntry *)
			mem_allocator.alloc(sizeof(MoccLockEntry), _row->get_part_id());
		return entry;
	}

	void Row_mocc_silo::return_entry(MoccLockEntry * entry) {
		mem_allocator.free(entry, sizeof(MoccLockEntry));
	}


	int Row_mocc_silo::get_owner_cnt() {
		return owner_cnt;
	}

#endif


#endif
