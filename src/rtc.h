#pragma once

#include "time.h"

void rtc_get_time(date_time_t* date);
void rtc_set_time(const date_time_t* date);