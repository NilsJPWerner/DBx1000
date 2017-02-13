// #include "global.h"
// #include "spinlock.h"

// #pragma once

// class table_t;
// class Catalog;
// class txn_man;
// struct TsReqEntry;

// #if CC_ALG==MOCC_SILO

// #define LOCK_BIT (1UL << 63)
// #define WRITE_BIT (1UL << 63)
// #define RTS_LEN (15)
// #define WTS_LEN (63 - RTS_LEN)
// #define WTS_MASK ((1UL << WTS_LEN) - 1)
// #define RTS_MASK (((1UL << RTS_LEN) - 1) << WTS_LEN)

// class Row_mocc_silo {
// public:
// 	void 				init(row_t * row);
// 	RC 					access(txn_man * txn, TsType type, row_t * local_row);

// 	// bool				validate(ts_t tid, bool in_write_set);
// 	void				write(row_t * data, uint64_t tid);

// 	void 				lock();
// 	void 				release();
// 	bool				try_lock();

// 	// New methods
// 	void				write_ptr(row_t * data, ts_t wts, char *& data_to_free);
// 	bool 				renew_lease(ts_t wts, ts_t rts);
// 	bool 				try_renew(ts_t wts, ts_t rts, ts_t &new_rts, uint64_t thd_id);

// 	RC					hot_access(txn_man * txn, access_t type, row_t * local_row);  //implement mcs later
// 	void 				hot_write(row_t * data, ts_t wts);
// 	void				hot_release();
// 	void				hot_renew(ts_t wts, ts_t commit_ts);

// 	ts_t 				get_wts();
// 	ts_t 				get_rts();
// 	void 				get_ts_word(bool &lock, uint64_t &rts, uint64_t &wts);
// private:
// 	row_t * 			_row;

// 	// New additions
// #if ATOMIC_WORD
// 	volatile uint64_t	_ts_word;
// #else
//  	pthread_mutex_t * 	_latch;
// 	ts_t 				_wts; // last write timestamp
// 	ts_t 				_rts; // end lease timestamp
// #endif
// 	spinlock *			_slock;
// };

// #endif
