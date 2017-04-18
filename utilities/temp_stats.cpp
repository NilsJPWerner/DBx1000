#include "temp_stats.h"
#include <ctime>

#include "global.h"
#include "helper.h"

PageTemperature::PageTemperature() {
    temperature = 0;
}

short
PageTemperature::get_temperature() {
    return temperature;
}

void
PageTemperature::increase_temperature() {
    // add locks in a bit
    srand(time(0));
    double probability = pow (2.0, -1.0 * temperature);
	bool increment = (rand() / (double) RAND_MAX) < probability;
    if (increment) {
        temperature++;
    }
}


PageTemperatures::PageTemperatures() {
    page_temps = {};
    lock = new pthread_mutex_t;
    // pthread_mutex_init(lock, NULL);
}

void
PageTemperatures::add_temperature(uint64_t row_addr) {
    // Locks to prevent write contention on simultaneous create same page temp
    pthread_mutex_lock(lock);
    unsigned long page = address_to_page(row_addr);
    if (!page_temps[page]) {
        page_temps[page] = new PageTemperature;
    }
    pthread_mutex_unlock(lock);
}

short
PageTemperatures::get_temperature(uint64_t row_addr) {
    PageTemperature* temp = address_to_temp(row_addr);
    if (!temp) cout << "HASHING FAILURE";
    return temp->get_temperature();
}

void
PageTemperatures::increase_temperature(uint64_t row_addr) {
    address_to_temp(row_addr)->increase_temperature();
}

unsigned long
PageTemperatures::num_active_pages() {
    return page_temps.size();
}

PageTemperature*
PageTemperatures::address_to_temp(uint64_t row_addr) {
    return page_temps[address_to_page(row_addr)];
}

unsigned long
PageTemperatures::address_to_page(uint64_t row_addr) {
    return row_addr / PAGE_SIZE;
}