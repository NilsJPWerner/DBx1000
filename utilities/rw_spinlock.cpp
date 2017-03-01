#include "rw_spinlock.h"
#include "atomic_def.h"
#include <unistd.h>
#include <stdio.h>

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
        // SHOULD I USE THIS OR ASSEMBLY:
        // #define cpu_relax() asm volatile("pause\n": : :"memory")
    }
}

void
rw_spinlock::spin_unlock() {
    // should I use a barrier here?
    // #define barrier() asm volatile("": : :"memory")
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
    // Again should I use assembly wait?
}

void
rw_spinlock::w_unlock() {
    spin_unlock();
}

void
rw_spinlock::w_trylock() {
    if (this->reader_count)
}


static int wr_trylock(rwspinlock *l)
{
    // Want no readers
    if (l->readers) return EBUSY;

    // Try to get write lock
    if (spin_trylock(&l->lock)) return EBUSY;

    if (l->readers)
    {
        // A reader started
        spin_unlock(&l->lock);
        return EBUSY;
    }

    return 0;
}
