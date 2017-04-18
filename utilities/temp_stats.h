#ifndef _TEMP_STAT_H_
#define _TEMP_STAT_H_

#include "global.h"
#include "helper.h"
#include <unordered_map>

class PageTemperature {
public:
    PageTemperature();
    short get_temperature();
    void increase_temperature();
private:
    short temperature;
};

class PageTemperatures {
public:
    PageTemperatures();
    void add_temperature(uint64_t row_addr);
    short get_temperature(uint64_t row_addr);
    void increase_temperature(uint64_t row_addr);
    unsigned long num_active_pages();
private:
    PageTemperature* address_to_temp(uint64_t row_addr);
    unsigned long address_to_page(uint64_t row_addr);
    std::unordered_map<unsigned long, PageTemperature*> page_temps;
    pthread_mutex_t * lock;
};

#endif