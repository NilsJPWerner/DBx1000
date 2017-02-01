#include "txn.h"
#include "row.h"
#include "row_mocc_silo.h"
#include "mem_alloc.h"
#include <thread>

// LOCK TESTING
#define ROWS_TO_LOCK 1

int locked_rows = 1;
//////


#if CC_ALG==MOCC_SILO

void
Row_mocc_silo::init(row_t * row)
{
	_row = row;
#if ATOMIC_WORD
	_tid_word = 0;
#else
	_latch = (pthread_mutex_t *) _mm_malloc(sizeof(pthread_mutex_t), 64);
	pthread_mutex_init( _latch, NULL );
	_tid = 0;
#endif

	// LOCK TESTING
	_test_lock = (pthread_mutex_t *) _mm_malloc(sizeof(pthread_mutex_t), 64);
	pthread_mutex_init( _test_lock, NULL );
}

RC
Row_mocc_silo::access(txn_man * txn, TsType type, row_t * local_row) {
#if ATOMIC_WORD
	// _test_record_id = 0;
	if (locked_rows <= ROWS_TO_LOCK) {
		_test_record_id = locked_rows;
		locked_rows++;
	}
	if (_test_record_id) {
		pthread_mutex_lock( _test_lock );
		_locked_by = std::this_thread::get_id();
	}
	uint64_t v = 0;
	uint64_t v2 = 1;
	while (v2 != v) {
		v = _tid_word;
		if (!(std::this_thread::get_id() == _locked_by)) {
			while (v & LOCK_BIT) {
				cout << std::this_thread::get_id();
				printf("Thread: is waiting for %lu\n", _test_record_id);
				PAUSE
				v = _tid_word;
			}
		}
		local_row->copy(_row);
		COMPILER_BARRIER
		v2 = _tid_word;
		printf("Thread made it here: %lu\n", _test_record_id);
	}
	txn->last_tid = v & (~LOCK_BIT);
	if (std::this_thread::get_id() == _locked_by) {
		pthread_mutex_unlock( _test_lock );
	}
#else
	local_row->copy(_row);
	txn->last_tid = _tid;
#endif
	return RCOK;
}

bool
Row_mocc_silo::validate(ts_t tid, bool in_write_set) {
#if ATOMIC_WORD
	uint64_t v = _tid_word;
	if (in_write_set)
		return tid == (v & (~LOCK_BIT));

	if (v & LOCK_BIT) {
		if (_test_record_id) {
			_row->manager->release();
		}
		return false;
	}
	else if (tid != (v & (~LOCK_BIT))) {
		if (_test_record_id) {
			_row->manager->release();
		}
		return false;
	}
	else {
		if (_test_record_id) {
			_row->manager->release();
		}
		return true;
	}
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

void
Row_mocc_silo::write(row_t * data, uint64_t tid) {
	_row->copy(data);
#if ATOMIC_WORD
	uint64_t v = _tid_word;
	M_ASSERT(tid > (v & (~LOCK_BIT)) && (v & LOCK_BIT), "tid=%ld, v & LOCK_BIT=%ld, v & (~LOCK_BIT)=%ld\n", tid, (v & LOCK_BIT), (v & (~LOCK_BIT)));
	_tid_word = (tid | LOCK_BIT);
#else
	_tid = tid;
#endif
}

void
Row_mocc_silo::lock() {
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

void
Row_mocc_silo::release() {
#if ATOMIC_WORD
	assert(_tid_word & LOCK_BIT);
	_tid_word = _tid_word & (~LOCK_BIT);
#else
	pthread_mutex_unlock( _latch );
#endif
}

bool
Row_mocc_silo::try_lock()
{
#if ATOMIC_WORD
	uint64_t v = _tid_word;
	if (v & LOCK_BIT) // already locked
		return false;
	return __sync_bool_compare_and_swap(&_tid_word, v, (v | LOCK_BIT));
#else
	return pthread_mutex_trylock( _latch ) != EBUSY;
#endif
}

#if ATOMIC_WORD
uint64_t
Row_mocc_silo::get_tid()
{
	assert(ATOMIC_WORD);
	return _tid_word & (~LOCK_BIT);
}
#endif

#endif
