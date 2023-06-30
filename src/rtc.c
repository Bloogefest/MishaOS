#include "rtc.h"

#include "terminal.h"
#include "io.h"
#include "sleep.h"

#define IO_RTC_INDEX 0x70
#define IO_RTC_TARGET 0x71

#define REG_SEC 0x00
#define REG_SEC_ALARM 0x01
#define REG_MIN 0x02
#define REG_MIN_ALARM 0x03
#define REG_HOUR 0x04
#define REG_HOUR_ALARM 0x05
#define REG_WEEK_DAY 0x06
#define REG_DAY 0x07
#define REG_MONTH 0x08
#define REG_YEAR 0x09
#define REG_A 0x0A
#define REG_B 0x0B
#define REG_C 0x0C
#define REG_D 0x0D

#define REGA_UIP (1 << 7)

#define REGB_HOURFORM (1 << 1)
#define REGB_DM (1 << 2)

static uint8_t rtc_read(uint8_t addr) {
    outb(IO_RTC_INDEX, addr);
    return inb(IO_RTC_TARGET);
}

static void rtc_write(uint8_t addr, uint8_t value) {
    outb(IO_RTC_INDEX, addr);
    outb(IO_RTC_TARGET, value);
}

static uint8_t bcd_to_bin(uint8_t value) {
    return (value & 0x0F) + (value >> 4) * 10;
}

static uint8_t bin_to_bcd(uint8_t value) {
    return ((value / 10) << 4) + (value % 10);
}

void rtc_get_time(date_time_t* date) {
    if (rtc_read(REG_A) & REGA_UIP) {
        pit_sleep(3);
    }

    uint8_t sec = rtc_read(REG_SEC);
    uint8_t min = rtc_read(REG_MIN);
    uint8_t hour = rtc_read(REG_HOUR);
    uint8_t week_day = rtc_read(REG_WEEK_DAY);
    uint8_t day = rtc_read(REG_DAY);
    uint8_t month = rtc_read(REG_MONTH);
    uint16_t year = rtc_read(REG_YEAR);

    uint8_t regb = rtc_read(REG_B);
    if (~regb & REGB_DM) {
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        hour = bcd_to_bin(hour);
        day = bcd_to_bin(day);
        month = bcd_to_bin(month);
        year = bcd_to_bin(year);
    }

    year += 2000;
    --week_day;

    date->sec = sec;
    date->min = min;
    date->hour = hour;
    date->day = day;
    date->month = month;
    date->year = year;
    date->week_day = week_day;
    date->tz_offset = local_time_zone;
}

void rtc_set_time(const date_time_t* date) {
    uint8_t sec = date->sec;
    uint8_t min = date->min;
    uint8_t hour = date->hour;
    uint8_t day = date->day;
    uint8_t month = date->month;
    uint8_t year = date->year - 2000;
    uint8_t week_day = date->week_day + 1;

    if (sec >= 60 || min >= 60 || hour >= 24 || day > 31 || month > 12 || year >= 100 || week_day > 7) {
        puts("rtc_set_time: invalid date");
        return;
    }

    uint8_t regb = rtc_read(REG_B);
    if (~regb & REGB_DM) {
        sec = bin_to_bcd(sec);
        min = bin_to_bcd(min);
        hour = bin_to_bcd(hour);
        day = bin_to_bcd(day);
        month = bin_to_bcd(month);
        year = bin_to_bcd(year);
    }

    if (rtc_read(REG_A) & REGA_UIP) {
        pit_sleep(3);
    }

    rtc_write(REG_SEC, sec);
    rtc_write(REG_MIN, min);
    rtc_write(REG_HOUR, hour);
    rtc_write(REG_WEEK_DAY, week_day);
    rtc_write(REG_DAY, day);
    rtc_write(REG_MONTH, month);
    rtc_write(REG_YEAR, year);
}