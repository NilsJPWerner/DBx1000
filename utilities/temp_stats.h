#ifndef _TEMP_STAT_H_
#define _TEMP_STAT_H_

#include "global.h"
#include "helper.h"
#include <unordered_map>

class PageTemperatures {
public:
    PageTemperatures();
    void add_temperature(uint64_t row_addr);
    unsigned short get_temperature(uint64_t row_addr);
    void increase_temperature(uint64_t row_addr);
    unsigned long num_active_pages();
private:
    std::unordered_map<unsigned long, unsigned short> page_temps;
    pthread_mutex_t * lock;
};

#endif