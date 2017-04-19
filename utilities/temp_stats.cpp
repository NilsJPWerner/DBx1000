#include "temp_stats.h"
#include <ctime>

#include "global.h"
#include "helper.h"

PageTemperatures::PageTemperatures() {
    page_temps = {};
    lock = new pthread_mutex_t;
}

void
PageTemperatures::add_temperature(uint64_t row_addr) {
    // Locks to prevent write contention on simultaneous create same page temp
    pthread_mutex_lock(lock);
    unsigned long page = row_addr / PAGE_SIZE;
    if (!page_temps[page]) {
        page_temps[page] = 0;
    }
    pthread_mutex_unlock(lock);
}

unsigned short
PageTemperatures::get_temperature(uint64_t row_addr) {
    return page_temps[row_addr / PAGE_SIZE];
}

void
PageTemperatures::increase_temperature(uint64_t row_addr) {
    srand(time(0));
    unsigned short temp = page_temps[row_addr / PAGE_SIZE];
    double probability = pow (2.0, -1.0 * temp);
	bool increment = (rand() / (double) RAND_MAX) < probability;
    if (increment) {
        page_temps[row_addr / PAGE_SIZE]++;
    }
}

unsigned long
PageTemperatures::num_active_pages() {
    return page_temps.size();
}