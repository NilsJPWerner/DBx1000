#pragma once

#include "row.h"

// TODO For simplicity, the txn hisotry for OCC is oganized as follows:
// 1. history is never deleted.
// 2. hisotry forms a single directional list.
//		history head -> hist_1 -> hist_2 -> hist_3 -> ... -> hist_n
//    The head is always the latest and the tail the youngest.
// 	  When history is traversed, always go from head -> tail order.

class txn_man;

class mocc_set_ent{
public:
	mocc_set_ent();
	UInt64 tn;
	txn_man * txn;
	UInt32 set_size;
	row_t ** rows;
	mocc_set_ent * next;
};

class MOptCC {
public:
	void init();
	RC validate(txn_man * txn);
	volatile bool lock_all;
	uint64_t lock_txn_id;
private:

	// per row validation similar to Hekaton.
	RC per_row_validate(txn_man * txn);

	// parallel validation in the original OCC paper.
	RC central_validate(txn_man * txn);
	bool test_valid(mocc_set_ent * set1, mocc_set_ent * set2);
	void get_invalid_rows(mocc_set_ent * set1, mocc_set_ent * set2, vector<UInt64> * invalid_rows);
	RC get_rw_set(txn_man * txni, mocc_set_ent * &rset, mocc_set_ent *& wset);

	// "history" stores write set of transactions with tn >= smallest running tn
	mocc_set_ent * history;
	mocc_set_ent * active;
	uint64_t his_len;
	uint64_t active_len;
	volatile uint64_t tnc; // transaction number counter
	pthread_mutex_t latch;
};
