#include "spinlock.h"
#include "atomic_def.h"
#include <unistd.h>
#include <stdio.h>

void
spinlock::init() {
	this->read_counter = 0;
	this->write_counter = 0;
}

void
spinlock::w_lock() {
	bool done = false;
	signed char notlock = 0;
	/*while (!done) {
		notlock = 0;
		done = COMPARE_AND_SWAP(&this->write_counter, &notlock, 1);
	}*/

	signed char r = ATOMIC_LOAD(&this->read_counter);
	while(true) {
		if (r == 0) {
			notlock = 0;
			done = COMPARE_AND_SWAP(&this->read_counter, &notlock, -MAXREADERS);
			if (done) {
				return;
			} else {
				r = ATOMIC_LOAD(&this->read_counter);
			}
		} else {
			r = ATOMIC_LOAD(&this->read_counter);
		}
	}

}

void
spinlock::w_unlock() {
	ATOMIC_ADD(&this->read_counter, MAXREADERS);
	//ATOMIC_SUB(&this->write_counter, 1);
}

void
spinlock::r_lock() {
	if (ATOMIC_ADD(&this->read_counter, 1 ) < 0) {
		while (ATOMIC_LOAD(&this->read_counter) < 0) {
		}
	}
}

void
spinlock::r_unlock() {
	ATOMIC_SUB(&this->read_counter, 1);
}
