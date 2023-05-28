#pragma once

#include <stdint.h>
#include <stddef.h>

typedef int32_t abs_time;

typedef struct date_time_s {
    int sec;
    int min;
    int hour;
    int day;
    int month;
    int year;
    int week_day;
    int year_day;
    int tz_offset;
} date_time_t;

#define TIME_STRING_SIZE 34

extern int local_time_zone;

void split_time(date_time_t* date, abs_time t, int tz_offset);
abs_time join_time(const date_time_t* date);
void format_time(char* str, const date_time_t* date);