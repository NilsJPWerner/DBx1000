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

    _hot_locked = false;
    _hot_locked_by = NULL;

#if RECORD_TEMP_STATS
	temp = 0;
#endif
}

RC Row_mocc_silo::access(txn_man * txn, TsType type, row_t * local_row, bool hot_record) {
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
	if (!hot_record)
		lock();
	local_row->copy(_row);
	txn->last_tid = _tid;
	if (!hot_record)
		release();
#endif
	return RCOK;
}

RC Row_mocc_silo::hot_lock(lock_t type, txn_man * txn) {
    RC rc = RCOK;
    if (try_lock(txn)) {
        _hot_locked = true;
        _hot_locked_by = txn;
        assert_lock();
    } else {
        // no wait abort
        rc = Abort;
    }
    return rc;
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
	if (!try_lock(txn))
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
    _hot_locked = false;
#if ATOMIC_WORD
	assert(_tid_word & LOCK_BIT);
	_tid_word = _tid_word & (~LOCK_BIT);
#else
	pthread_mutex_unlock( _latch );
#endif
}

// Attempts to lock. Locks if free and returns true
// If locked check if hot locked by current txn
bool Row_mocc_silo::try_lock(txn_man * txn) {
#if ATOMIC_WORD
	uint64_t v = _tid_word;
	if (v & LOCK_BIT) // already locked
		goto hot_lock_check;
	return __sync_bool_compare_and_swap(&_tid_word, v, (v | LOCK_BIT));
#else
	if (pthread_mutex_trylock( _latch ) != EBUSY) {
        return true;
    } else {
		// #if LOCK_STRATEGY == "wait-die"

		// #endif
        goto hot_lock_check;
    }
#endif
hot_lock_check:
    return (_hot_locked && _hot_locked_by == txn);
}

bool Row_mocc_silo::conflict_lock(lock_t l1, lock_t l2) {
	if (l1 == LOCK_NONE || l2 == LOCK_NONE)
		return false;
    else if (l1 == LOCK_EX || l2 == LOCK_EX)
        return true;
	else
		return false;
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

#if RECORD_TEMP_STATS
	//Will get the temperature of the record
	unsigned int
	Row_mocc_silo::get_temp() {
		return temp;
	}

	void Row_mocc_silo::update_temp_stat() {
		temp++;
	}
#endif


#endif
