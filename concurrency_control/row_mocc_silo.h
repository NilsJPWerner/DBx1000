#pragma once

class table_t;
class Catalog;
class txn_man;
struct TsReqEntry;

#if CC_ALG==MOCC_SILO
#define LOCK_BIT (1UL << 63)

#if (RECORD_TEMP_STATS && HOT_LOCK_RECORDS)
	struct MoccLockEntry {
		lock_t type;
		txn_man * txn;
		MoccLockEntry * next;
		MoccLockEntry * prev;
	};
#endif

class Row_mocc_silo {
public:
	void 				init(row_t * row);
	RC 					access(txn_man * txn, TsType type, row_t * local_row);

	bool				validate(txn_man * txn, ts_t tid, bool in_write_set);
	void				write(row_t * data, uint64_t tid);

	void 				lock();
	void 				release();
	bool				try_lock();
	uint64_t 			get_tid();
	void 				assert_lock();

#if RECORD_TEMP_STATS
	#if HOT_LOCK_RECORDS
		RC 				hot_access(txn_man * txn, lock_t type, row_t * local_row);
		RC 				hot_access(txn_man * txn, lock_t type, uint64_t* &txnids, int &txncnt, row_t * local_row);
		RC 				hot_release(txn_man * txn);

		int 			get_owner_cnt();
	#endif

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

#if RECORD_TEMP_STATS
	#if STAT_TYPE == PER_RECORD_TEMPS
		unsigned short  temp;
	#endif

	#if HOT_LOCK_RECORDS
		lock_t lock_type;
		UInt32 owner_cnt;
    	UInt32 waiter_cnt;

		bool		conflict_lock(lock_t l1, lock_t l2);
		MoccLockEntry * get_entry();
		void 		return_entry(MoccLockEntry * entry);

		MoccLockEntry * owners;
		MoccLockEntry * waiters_head;
		MoccLockEntry * waiters_tail;
	#endif
#endif
};

#endif
