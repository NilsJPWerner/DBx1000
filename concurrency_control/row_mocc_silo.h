#pragma once

class table_t;
class Catalog;
class txn_man;
struct TsReqEntry;

#if CC_ALG==MOCC_SILO
#define LOCK_BIT (1UL << 63)

// struct LockEntry {
//     lock_t type;
//     txn_man * txn;
// 	LockEntry * next;
// 	LockEntry * prev;
// };

class Row_mocc_silo {
public:
	void 				init(row_t * row);
	RC 					access(txn_man * txn, TsType type, row_t * local_row, bool hot_record);

	bool				validate(txn_man * txn, ts_t tid, bool in_write_set);
	void				write(row_t * data, uint64_t tid);

	void 				lock();
	void 				release();
	bool				try_lock(txn_man * txn);
	uint64_t 			get_tid();

	void 				assert_lock();

	bool				conflict_lock(lock_t l1, lock_t l2);

	RC					hot_lock(lock_t type, txn_man * txn);

	#if RECORD_TEMP_STATS
		unsigned short		get_temp();
		void 				update_temp_stat();
	#endif

private:
#if ATOMIC_WORD
	volatile uint64_t	_tid_word;
#else
 	pthread_mutex_t * 	_latch;
	ts_t 				_tid;
#endif
	row_t * 			_row;

	bool 				_hot_locked;
	txn_man * 			_hot_locked_by;

#if RECORD_TEMP_STATS
	#if STAT_TYPE == "per-record"
		temp++;
	#elif STAT_TYPE == "global-hashtable"
		// No instance varaibles needed for global temps
	#else
		#error "Specified stat type is not implemented"
	#endif
#endif
};

#endif
