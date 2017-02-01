#ifndef _ATOMIC_DEF_H
#define _ATOMIC_DEF_H

#define COMPARE_AND_SWAP(valptr, expectptr, desire) \
	__atomic_compare_exchange_n(valptr, expectptr, desire, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)

#define ATOMIC_SUB(valptr, subval) \
	__atomic_sub_fetch(valptr, subval, __ATOMIC_RELAXED)

#define ATOMIC_ADD(valptr, addval) \
	__atomic_add_fetch(valptr, addval, __ATOMIC_RELAXED)

#define ATOMIC_LOAD(valptr) \
	__atomic_load_n(valptr, __ATOMIC_RELAXED)

#define ATOMIC_STORE(valptr, storeval) \
	__atomic_store_n(valptr, storeval, __ATOMIC_RELAXED)

#define ATOMIC_EXCHANGE(valptr, exchangeval) \
	__atomic_exchange_n(valptr, exchangeval, __ATOMIC_RELAXED)

#endif
