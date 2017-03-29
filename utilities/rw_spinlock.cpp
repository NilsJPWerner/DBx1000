#include "rw_spinlock.h"
#include "atomic_def.h"
#include <unistd.h>
#include <stdio.h>

#define barrier() asm volatile("": : :"memory");

void
rw_spinlock::init() {
    this->lock = 0;
    this->reader_count = 0;
}

void
rw_spinlock::spin_lock() {
    while (true) {
        if (!ATOMIC_EXCHANGE(&this->lock, EBUSY)) return;
        while (this->lock) usleep(1);
    }
}

void
rw_spinlock::spin_unlock() {
    barrier();
    this->lock = 0;
}

int
rw_spinlock::spin_trylock() {
    return ATOMIC_EXCHANGE(&this->lock, EBUSY);
}

void
rw_spinlock::w_lock() {
    // Get write lock
    spin_lock();
    // Wait for readers
    while (this->reader_count) usleep(1);
}

void
rw_spinlock::w_unlock() {
    spin_unlock();
}

int
rw_spinlock::w_trylock() {
    // Test for any readers
    if (this->reader_count) return EBUSY;

    // Try to get write lock
    if (spin_trylock()) return EBUSY;

    // Test to see if a reader has started while
    // in the middle of w locking
    if (this->reader_count) {
        spin_unlock();
        return EBUSY;
    }
    // Success
    return 0;
}

void
rw_spinlock::r_lock() {
    while (true) {
        // Take a read lock
        ATOMIC_ADD(&this->reader_count, 1);
        // Return if lock is succesful
        if (!this->lock) return;
        // Remove read lock if failed
        ATOMIC_SUB(&this->reader_count, 1);
        // Wait until w lock is removed
        while (this->lock) usleep(1);
    }
}

void
rw_spinlock::r_unlock() {
    ATOMIC_SUB(&this->reader_count, 1);
}

int
rw_spinlock::r_trylock() {
    // Speculatively take r lock and return if success
    ATOMIC_ADD(&this->reader_count, 1);
    if (!this->lock) return 0;

    // Return EBUSY if failed
    ATOMIC_SUB(&this->reader_count, 1);
	return EBUSY;
}

int
rw_spinlock::r_upgradelock() {
    // Try to convert into a write lock
	if (spin_trylock()) return EBUSY;

	// I'm no longer a reader
	ATOMIC_SUB(&this->reader_count, 1);

	// Wait for all other readers to finish
	while (this->reader_count) usleep(1);
	return 0;
}