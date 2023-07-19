#include "time.h"

#include <lib/string.h>
#include <lib/stdlib.h>

int local_time_zone = -7 * 60;

void split_time(date_time_t* date, abs_time t, int tz_offset) {
    t += tz_offset * 60;

    static const int month_start[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
    static const int leap_month_start[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

    int epoch_days = t / (24 * 60 * 60);
    int day_secs = t % (24 * 60 * 60);

    int sec = day_secs % 60;
    int min = (day_secs % 3600) / 60;
    int hour = day_secs / 3600;

    int epoch_years = (epoch_days - (epoch_days + 365) / 1460) / 365;
    int year_day = epoch_days - (epoch_years * 365 + (epoch_years + 1) / 4);

    int year = 1970 + epoch_years;

    const int* mstart = year & 3 ? month_start : leap_month_start;

    int month = 1;
    while (year_day >= mstart[month]) {
        ++month;
    }

    int day = 1 + year_day - mstart[month - 1];
    int week_day = (epoch_days + 4) % 7;

    date->sec = sec;
    date->min = min;
    date->hour = hour;
    date->day = day;
    date->month = month;
    date->year = year;
    date->week_day = week_day;
    date->year_day = year_day;
    date->tz_offset = tz_offset;
}

abs_time join_time(const date_time_t* date) {
    return date->sec + date->min * 60 + date->hour * 3600
        + date->year_day * 86400 + (date->year - 70) * 31536000
        + ((date->year - 69) / 4) * 86400 - ((date->year - 1) / 100) * 86400
        + ((date->year + 299) / 400) * 86400;
}

void format_time(char* str, const date_time_t* date) {
    static const char* week_days[] = {
            "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };

    static const char* months[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    uint32_t w = date->week_day;
    uint32_t m = date->month - 1;

    const char* week_day = w < 7 ? week_days[w] : "???";
    memcpy(str, (void*) week_day, strlen(week_day));
    str += strlen(week_day);
    *str++ = ',';
    *str++ = ' ';
    if (date->day < 10) {
        *str++ = '0';
    }

    str = itoa(date->day, str, 10);
    *str++ = ' ';
    const char* month = m < 12 ? months[m] : "???";
    memcpy(str, (void*) month, strlen(month));
    str += strlen(month);
    *str++ = ' ';
    str = itoa(date->year, str, 10);
    *str++ = ' ';
    if (date->hour < 10) {
        *str++ = '0';
    }

    str = itoa(date->hour, str, 10);
    *str++ = ':';
    if (date->min < 10) {
        *str++ = '0';
    }

    str = itoa(date->min, str, 10);
    *str++ = ':';
    if (date->sec < 10) {
        *str++ = '0';
    }

    str = itoa(date->sec, str, 10);
    *str++ = ' ';
    *str++ = '+';

    int tzh = date->tz_offset / 60;
    int tzm = date->tz_offset % 60;
    if (tzh < 10) {
        *str++ = '0';
    }

    str = itoa(tzh, str, 10);
    *str++ = ':';
    if (tzm < 10) {
        *str++ = '0';
    }

    itoa(tzm, str, 10);
}