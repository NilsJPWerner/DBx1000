#define _RWSPINLOCK_H

#define EBUSY 1
#define MAXREADERS 1 << 6

class rw_spinlock {
public:
	void init();

    void spin_lock();
    void spin_unlock();
    int  spin_trylock();

	void w_lock();
	void w_unlock();
    int  w_trylock();

	void r_lock();
	void r_unlock();
    int  r_trylock();

    int r_upgradelock();

private:
    char lock;
	signed char reader_count;
};
