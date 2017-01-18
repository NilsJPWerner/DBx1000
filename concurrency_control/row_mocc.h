#ifndef ROW_MOCC_H
#define ROW_MOCC_H

class table_t;
class Catalog;
class txn_man;
struct TsReqEntry;

class Row_mocc {
public:
	void 				init(row_t * row);
	RC 					access(txn_man * txn, TsType type);
	void 				latch();
	// ts is the start_ts of the validating txn
	bool				validate(uint64_t ts);
	void				write(row_t * data, uint64_t ts);
	void 				release();
private:
 	pthread_mutex_t * 	_latch;
	bool 				blatch;

	row_t * 			_row;
	// the last update time
	ts_t 				wts;

	long unsigned int 	_virtual_page;
};

#endif
