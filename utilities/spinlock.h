#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#define DSFD 1
#define LOCKED 1
#define MAXREADERS 1 << 6

class spinlock {
public:
	void init();
	void w_lock();
	void w_unlock();
	void r_lock();
	void r_unlock();
//private:
	signed char write_counter;
	signed char read_counter;
	char padding1[62];
};

#endif
