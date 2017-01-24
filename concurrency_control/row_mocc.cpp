#include "txn.h"
#include "row.h"
#include "row_mocc.h"
#include "mem_alloc.h"
#include "manager.h"

void
Row_mocc::init(row_t * row) {
	_row = row;
	int part_id = row->get_part_id();
	_latch = (pthread_mutex_t *)
		mem_allocator.alloc(sizeof(pthread_mutex_t), part_id);
	pthread_mutex_init( _latch, NULL );
	wts = 0;
	blatch = false;

	glob_manager->add_temp_stat((uint64_t)_row);
}

RC
Row_mocc::access(txn_man * txn, TsType type) {
	RC rc = RCOK;
	pthread_mutex_lock( _latch );
	if (type == R_REQ) {
		if (txn->start_ts < wts)
			rc = Abort;
		else {
			txn->cur_row->copy(_row);
			rc = RCOK;
		}
	} else
		assert(false);
	pthread_mutex_unlock( _latch );
	return rc;
}

void
Row_mocc::latch() {
	pthread_mutex_lock( _latch );
}

bool
Row_mocc::validate(uint64_t ts) {
	if (ts < wts) return false;
	else return true;
}

void
Row_mocc::write(row_t * data, uint64_t ts) {
	_row->copy(data);
	if (PER_ROW_VALID) {
		assert(ts > wts);
		wts = ts;
	}
}

void
Row_mocc::release() {
	pthread_mutex_unlock( _latch );
}
