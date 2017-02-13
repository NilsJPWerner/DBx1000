// #include "txn.h"
// #include "row.h"
// #include "row_silo.h"
// #include "mem_alloc.h"

// #if CC_ALG==MOCC_SILO

// // USE NON ATOMIC FOR NOW

// void
// Row_mocc_silo::init(row_t * row) {
// 	_row = row;
// #if ATOMIC_WORD
// 	_ts_word = 0;
// #else
// 	_latch = (pthread_mutex_t *) _mm_malloc(sizeof(pthread_mutex_t), 64);
// 	pthread_mutex_init( _latch, NULL );
// 	_wts = 0;
// 	_rts = 0;
// #endif
// 	// spinlock initialization
// 	// this->_slock = (spinlock *) _mm_malloc(sizeof(spinlock), 64);
// 	// this->_slock->init();
// }

// /// LOCKING LOGIC ////

// void
// Row_mocc_silo::lock() {
// #if ATOMIC_WORD
// 	uint64_t v = _ts_word;
// 	while ((v & LOCK_BIT) ||  !__sync_bool_compare_and_swap(&_ts_word, v, v | LOCK_BIT)) {
// 		PAUSE
// 		v = _ts_word;
// 	}
// #else
// 	pthread_mutex_lock( _latch );
// 	bool conflict = lock_conflict(lock_type, type);
// 	if (conflict) {
// 		rc = Abort;
// 		pthread_mutex_unlock( _latch );
// 	} else {

// 	}
// #endif
// }

// bool
// Row_mocc_silo::lock_conflict(lock_t l1, lock_t l2) {
// 	if (l1 == LOCK_NONE || l2 == LOCK_NONE)
// 		return false;
//     else if (l1 == LOCK_EX || l2 == LOCK_EX)
//         return true;
// 	else
// 		return false;
// }

// LockEntry * Row_lock::get_entry() {
// 	LockEntry * entry = (LockEntry *)
// 		mem_allocator.alloc(sizeof(LockEntry), _row->get_part_id());
// 	return entry;
// }

// void Row_lock::return_entry(LockEntry * entry) {
// 	mem_allocator.free(entry, sizeof(LockEntry));
// }

// //////////

// RC
// Row_mocc_silo::access(txn_man * txn, TsType type, row_t * local_row, bool hot_record) {
// #if ATOMIC_WORD
// 	uint64_t v = 0;
// 	uint64_t v2 = 1;
// 	uint64_t lock_mask = LOCK_BIT;
// 	if (WRITE_PERMISSION_LOCK && type == P_REQ)
// 		lock_mask = WRITE_BIT;

// 	while ((v2 | RTS_MASK) != (v | RTS_MASK)) {
// 		v = _ts_word;
// 		while (v & lock_mask) {
// 			PAUSE
// 			v = _ts_word;
// 		}
// 		local_row->copy(_row);
// 		COMPILER_BARRIER
// 		v2 = _ts_word;
// 	}
// 	txn->last_wts = v & WTS_MASK;
// 	txn->last_rts = ((v & RTS_MASK) >> WTS_LEN) + txn->last_wts;
// #else
// 	if (!hot_record)
// 		lock();
// 	local_row->copy(_row);
// 	txn->last_wts = _wts;
// 	txn->last_rts = _rts;
// 	if (!hot_record)
// 		release();
// #endif
// 	return RCOK;
// }

// // bool
// // Row_mocc_silo::validate(ts_t tid, bool in_write_set) {
// // #if ATOMIC_WORD
// // 	uint64_t v = _tid_word;
// // 	if (in_write_set)
// // 		return tid == (v & (~LOCK_BIT));

// // 	if (v & LOCK_BIT)
// // 		return false;
// // 	else if (tid != (v & (~LOCK_BIT)))
// // 		return false;
// // 	else
// // 		return true;
// // #else
// // 	if (in_write_set)
// // 		return tid == _tid;
// // 	if (!try_lock())
// // 		return false;
// // 	bool valid = (tid == _tid);
// // 	release();
// // 	return valid;
// // #endif
// // }

// void
// Row_mocc_silo::write(row_t * data, uint64_t tid) {
// #if ATOMIC_WORD
// 	// uint64_t v = _tid_word;
// 	// M_ASSERT(tid > (v & (~LOCK_BIT)) && (v & LOCK_BIT), "tid=%ld, v & LOCK_BIT=%ld, v & (~LOCK_BIT)=%ld\n", tid, (v & LOCK_BIT), (v & (~LOCK_BIT)));
// 	// _tid_word = (tid | LOCK_BIT);
// #else
// 	_wts = wts;
// 	_rts = rts;
// 	_row->copy(data);
// #endif
// }

// bool
// Row_mocc_silo::try_lock() {
// #if ATOMIC_WORD
// 	if (_ts_word & LOCK_BIT) // already locked
// 		return false;
// 	return __sync_bool_compare_and_swap(&_ts_word, _ts_word, (_ts_word | LOCK_BIT));
// #else
// 	return pthread_mutex_trylock( _latch ) != EBUSY;
// #endif
// }

// void
// Row_mocc_silo::release() {
// #if ATOMIC_WORD
// 	assert(_ts_word & LOCK_BIT);
// 	_ts_word = _ts_word & (~LOCK_BIT);
// #else
// 	pthread_mutex_unlock( _latch );
// #endif
// }

// ///////////////////////////////////////////
// ///////////////////////////////////////////

// ts_t
// Row_mocc_silo::get_wts() {
// #if ATOMIC_WORD
// 	return _ts_word & WTS_MASK;
// #else
// 	return _wts;
// }

// ts_t
// Row_mocc_silo::get_rts() {
// #if ATOMIC_WORD
// 	return ((v & RTS_MASK) >> WTS_LEN) + (v & WTS_MASK);  // Why do we want the WTS as well?
// #else
// 	return _rts;
// }

// void
// Row_mocc_silo::get_ts_word(bool &lock, uint64_t &rts, uint64_t &wts) {
// 	assert(ATOMIC_WORD);
// 	uint64_t v = _ts_word;
// 	lock = ((v & LOCK_BIT) != 0);
// 	wts = v & WTS_MASK;
// 	rts = ((v & RTS_MASK) >> WTS_LEN)  + (v & WTS_MASK);
// }

// // Remove locking in non atomic part
// RC
// Row_mocc_silo::hot_access(txn_man *txn, access_t type, row_t * local_row) {
// 	if (type == RD) {
// 		// Read Lock
// 		this->_slock->r_lock();
// 	} else {
// 		// WriteLock
// 		this->_slock->w_lock();
// 	}
// #if ATOMIC_WORD
// 	local_row->copy(_row);
// 	txn->last_wts = _ts_word & WTS_MASK
// 	txn->last_rts = ((_ts_word & RTS_MASK) >> WTS_LEN) + _ts_word & WTS_MASK;
// #else
// 	lock();  // remove this
// 	local_row->copy(_row);
// 	txn->last_wts = _wts;
// 	txn->last_rts = _rts;
// 	release();  // and this
// 	return RCOK;
// #endif
// }

// void
// Row_mocc_silo::hot_release(access_t type) {
// 	if (type == RD) {
// 		// Read Unlock
// 		this->_slock->r_unlock();
// 	} else {
// 		// Write Unlock
// 		this->_slock->w_unlock();
// 	}
// }

// void
// Row_mocc_silo::hot_write(row_t * data, uint64_t tid) {
// #if ATOMIC_WORD
// 	_ts_word |= LOCK_BIT;
//   	uint64_t v = _ts_word;
//   	v &= ~(RTS_MASK | WTS_MASK); // clear wts and rts.
// 	v |= wts;
// 	_ts_word = v;
// 	_row->copy(data);
// 	_ts_word &= (~LOCK_BIT);
// #else
// 	// DO I NEED TO LOCK HERE?
// 	_wts = wts;
// 	_rts = rts;
// 	_row->copy(data);
// #endif
// }

// #endif

// bool
// Row_mocc_silo::renew_lease(ts_t wts, ts_t rts) {
// #if ATOMIC_WORD
// 	return true;
// #else
// 	if (_wts != wts) {
// 		return false;
// 	}
// 	_rts = rts;
// }

// bool
// Row_mocc_silo::try_renew(ts_t wts, ts_t rts, ts_t &new_rts, uint64_t thd_id) {
// #if ATOMIC_WORD
// 	// stuff
// #else
// 	if (_wts != wts)
// 		return false;
// 	int lock_response = pthread_mutex_trylock(_latch);
// 	if (lock_response == EBUSY)
// 		return false;
// 	if (_wts != wts)
// 		pthread_mutex_unlock(_latch);
// 		return false;
// 	if (rts > _rts)
// 		_rts = rts;
// 	pthread_mutex_unlock(_latch);
// 	new_rts = rts;
// 	return true
// #endif
// }

// void
// Row_mocc_silo::hot_renew(ts_t wts, ts_t commit_ts) {
// #if ATOMIC_WORD
// 	ts_t delta_rts = commit_ts - wts;
// 	if (delta_rts < ((_ts_word & RTS_MASK) >> WTS_LEN)) // the rts has already been extended.
// 		return;
// 	if (delta_rts >= (1 << RTS_LEN)) {
// 		assert(false);
// 		uint64_t delta = (delta_rts & ~((1 << RTS_LEN) - 1));
// 		delta_rts &= ((1 << RTS_LEN) - 1);
// 		wts += delta;
// 	}
// 	uint64_t v2 = 0;
// 	v2 |= wts;
// 	v2 |= (delta_rts << WTS_LEN);
// 	_ts_word =  v2;
// #else
// 	// INCOMPLETE
// #endif
// }