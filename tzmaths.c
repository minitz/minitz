
/* minitz - MINiaturised International TimeZone database v1.0
   
   Written by Nicholas Vinen. Please see README and LICENSE files.
   
   Time zone offset / daylight savings manipulation routines. */


#include "tzmaths.h"
#define INCLUDE_TZ_DATA
#include "tz_coords.h"


typedef struct {
  unsigned long flag;
  unsigned char start_month;  // 1-12
  signed char start_week;     // 1 = first, 2 = second, -1 = last, -2 = second-last, etc.
  unsigned char start_dow;    // 0-7, 0 = Sunday
  signed char start_hour;     // non-DST hour, can be negative if changeover occurs the day before
  unsigned char finish_month; // 1-12
  signed char finish_week;    // 1 = first, 2 = second, -1 = last, -2 = second-last, etc.
  unsigned char finish_dow;   // 0-7, 0 = Sunday
  signed char finish_hour;    // non-DST hour, can be negative if changeover occurs the day before
} daylight_savings_info;
            
#define DAY_SUN         0
#define DAY_MON         1
#define DAY_FRI         5
#define DAY_SAT         6
#define WEEK_1ST        1
#define WEEK_2ND        2
#define WEEK_3RD        3
#define WEEK_4TH        4
#define WEEK_LAST      -1
#define WEEK_EQUI       0
#define EXCEPT_RAMADAN  1
#define EXCEPT_CARNAVAL 2
#define EXCEPT_ROSH_HAS 4

static const daylight_savings_info daylight_savings[] = {
  { DST_AU,                       10, WEEK_1ST,  DAY_SUN,   2,  4, WEEK_1ST,  DAY_SUN,  2 },
  { DST_NZ,                        9, WEEK_1ST,  DAY_SUN,   2,  4, WEEK_1ST,  DAY_SUN,  2 },
  { DST_BR | EXCEPT_CARNAVAL,     10, WEEK_3RD,  DAY_SUN,   0,  2, WEEK_3RD,  DAY_SUN, -1 },
  { DST_PAR,                      10, WEEK_1ST,  DAY_SUN,  -1,  4, WEEK_2ND,  DAY_SUN,  0 },
  { DST_URU,                      10, WEEK_1ST,  DAY_SUN,  -1,  3, WEEK_2ND,  DAY_SUN,  0 },
  { DST_FIJI,                     11, WEEK_1ST,  DAY_SUN,   2,  1, WEEK_3RD,  DAY_SUN,  2 },
  { DST_NA,                        3, WEEK_2ND,  DAY_SUN,   1, 11, WEEK_1ST,  DAY_SUN,  1 },
  { DST_EU,                        3, WEEK_LAST, DAY_SUN,   1, 10, WEEK_LAST, DAY_SUN,  1 },
  { DST_MX,                        4, WEEK_1ST,  DAY_SUN,   2, 10, WEEK_LAST, DAY_SUN,  1 },
  { DST_GNL,                       3, WEEK_LAST, DAY_SUN,  -2, 10, WEEK_LAST, DAY_SUN, -2 },
  { DST_MOROCCO | EXCEPT_RAMADAN,  3, WEEK_LAST, DAY_SUN,   2, 10, WEEK_LAST, DAY_SUN,  2 },
  { DST_EGYPT   | EXCEPT_RAMADAN,  4, WEEK_LAST, DAY_FRI,   0,  9, WEEK_LAST, DAY_FRI,  0 },
  { DST_ME,                        3, WEEK_LAST, DAY_FRI,   0, 10, WEEK_LAST, DAY_FRI,  0 },
  { DST_IS      | EXCEPT_ROSH_HAS, 3, WEEK_LAST, DAY_SUN, -46, 10, WEEK_LAST, DAY_SUN,  1 },
  { DST_PAL,                       3, WEEK_LAST, DAY_FRI,   0, 10, WEEK_4TH,  DAY_FRI,  0 },
  { DST_NAM,                       9, WEEK_1ST,  DAY_SUN,   2,  4, WEEK_1ST,  DAY_SUN,  1 },
  { DST_SAMOA,                     9, WEEK_LAST, DAY_SUN,   0,  4, WEEK_1ST,  DAY_SAT,  3 },
  { DST_IRAN,                      3, WEEK_EQUI,       1,   0,  9, WEEK_EQUI,       1, -1 }
};

// Easter dates for years 2000-2099. Positive for April day, negative for March.
static const signed char easter_dates[100] = { 23, 15, -31, 20, 11, -27, 16, 8, -23, 12,
                                               4, 24, 8, -31, 20, 5, -27, 16, 1, 21,
                                               12, 4, 17, 9, -31, 20, 5, -28, 16, 1,
                                               21, 13, -28, 17, 9, -25, 13, 5, 25, 10,
                                               1, 21, 6, -29, 17, 9, -25, 14, 5, 18,
                                               10, 2, 21, 6, -29, 18, 2, 22, 14, -30,
                                               18, 10, -26, 15, 6, -29, 11, 3, 22, 14,
                                               -30, 19, 10, -26, 15, 7, 19, 11, 3, 23,
                                               7, -30, 19, 4, -26, 15, -31, 20, 11, 3,
                                               16, 8, -30, 12, 4, 24, 15, -31, 20, 12 };

// Ramadan in the year 2000 begins on November 28th and ends on December 28th.
// To get the next Ramadan, add one year, then subtract value #0. Repeat for future Ramadans.
// Goes up to the year 2099. Ramadan lasts for 30 days except for instances where 64 has been
// added, which last for 29 days. The addition is to the previous value, ie, the first 29-day
// Ramadan is in 2004.
static const unsigned char ramadan_dates[102] = { 11, 11, 10, 11, 75, 11, 11, 11, 11, 11,
                                                  10, 12, 11, 10, 75, 11, 11, 11, 10, 12,
                                                  11, 10, 11, 12, 10, 11, 10, 11, 12, 10,
                                                  11, 11, 11, 11, 11, 10, 12, 10, 11, 11,
                                                  11, 11, 11, 10, 12, 10, 11, 11, 11, 11,
                                                  11, 10, 12, 11, 10, 11, 11, 11, 11, 10,
                                                  11, 12, 10, 11, 11, 11, 11, 10, 11, 12,
                                                  10, 11, 11, 11, 11, 10, 11, 12, 10, 11,
                                                  11, 11, 11, 11, 10, 12, 10, 11, 11, 11,
                                                  11, 11, 10, 11, 12, 10, 11, 10, 12, 11,
                                                  10, 11 };

// Day of September on which Rosh Hashanah falls for the years 2000-2099.
// 64 is added if it's in October instead.
static const unsigned char rosh_hashanah_dates[100] = { 30, 18,  7, 27, 16, 68, 23, 13, 30, 19,
                                                         9, 29, 17,  5, 25, 14, 67, 21, 10, 30,
                                                        19,  7, 26, 16, 67, 23, 12, 66, 21, 10,
                                                        28, 18,  6, 24, 14, 68, 22, 10, 30, 19,
                                                         8, 26, 15, 69, 22, 12, 65, 21,  8, 27,
                                                        17,  7, 24, 13, 67, 23, 11, 29, 19,  8,
                                                        25, 15, 69, 24, 11, 65, 20, 10, 27, 16,
                                                         6, 24, 13, 66, 22, 10, 28, 18,  8, 26,
                                                        14, 68, 24, 13, 30, 20,  9, 27, 16,  5,
                                                        25, 13, 66, 21, 11, 29, 17,  7, 27, 15 };

static const unsigned char days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static unsigned char get_days_in_month(unsigned char month, unsigned int year) {
    if( month == 2 && !(year&3) && ((year%100) || !(year%400)) )
        return 29;
    else
        return days_in_month[month-1];
}
static const unsigned char dow_table[12] = { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
static unsigned char get_dow(unsigned char day, unsigned char month, unsigned char year) {
    return (day + dow_table[month-1] + year + (year>>2) + 6 - (month <= 2 && !(year&3) ? 1 : 0)) % 7;
}

void add_timezone_offset(int* time, int* date, int offset) {
  int seconds = *time%100;
  int minutes = (*time/100)%100;
  int hours = *time/10000;
  int year = *date%100;
  int month = (*date/100)%100;
  int day = *date/10000;

  minutes += offset%100;
  if( minutes > 60 ) {
    hours += 1;
    minutes -= 60;
  }
  hours += offset/100;
  if( hours > 24 ) {
    day += 1;
    hours -= 24;
  }
  if( day > get_days_in_month(month, year) ) {
    if( ++month > 12 ) {
      month -= 12;
      year += 1;
    }
  }

  *time = seconds+minutes*100+hours*10000;
  *date = year+month*100+day*10000;
}

void add_sub_days(unsigned char* day, unsigned char* month, unsigned char* year, int num_days) {
  while( num_days > 0 ) {
    unsigned char days = get_days_in_month(*month, *year);
    if( num_days <= (days - *day) ) {
      *day += num_days;
      return;
    }
    num_days -= (days - *day + 1);
    if( ++*month == 13 ) {
      *month = 1;
      ++*year;
    }
  }
  while( num_days < 0 ) {
    if( -num_days < *day ) {
      *day += num_days;
      return;
    }
    num_days += *day;
    if( --*month == 0 ) {
      *month = 12;
      --*year;
    }
    *day = get_days_in_month(*month, *year);
  }
}


// returns the day of March of the vernal equinox for years 2000-2099
static unsigned char get_vernal_equinox(unsigned char year) {
  return year == 3 || (year < 64 && (year&3) == 3) || (year < 36 && (year&3) == 2) ? 21 : 20;
}

// returns 1 if a[hour, day, month, year] < b[hour, dow, week, month, year]
static unsigned char is_before(unsigned char hour_a, unsigned char day_a, signed char hour_b, unsigned char dow_b, signed char week_b, unsigned char month, unsigned char year, unsigned char except) {
  int first_dow = get_dow(1, month, year);
  int hour = hour_b;
  unsigned char orig_month = month;
  unsigned char orig_year = year;
  int day;

  if( week_b == WEEK_EQUI ) {
    // special case for Iran, day is relative to the day of the Vernal Equinox
    day = get_vernal_equinox(year) + dow_b;
  } else if( week_b > 0 ) {
    day = 1;
    while( first_dow != dow_b ) {
      ++day;
      if( ++first_dow == 7 )
        first_dow = 0;
    }
    day += (week_b - 1) * 7;
  } else {
    int last_dow;

    day = get_days_in_month(month, year);
    last_dow = (first_dow + day-1) % 7;
    while( last_dow != dow_b ) {
      --day;
      if( last_dow == 0 )
        last_dow = 6;
      else
        --last_dow;
    }
    day += (week_b + 1) * 7;
  }

  while( hour < 0 ) {
    if( --day == 0 ) {
      if( --month == 0 ) {
        month = 12;
        --year;
      }
      day = get_days_in_month(month, year);
    }
    hour += 24;
  }
  while( hour >= 24 ) {
    if( ++day > get_days_in_month(month, year) ) {
      day = 1;
      if( ++month == 13 ) {
        month = 1;
        ++year;
      }
    }
    hour -= 24;
  }

  if( except&EXCEPT_CARNAVAL ) {
    unsigned char easter_day, easter_month, carnaval_start_day, carnaval_finish_day, carnaval_start_month, carnaval_finish_month, carnaval_year;
    if( easter_dates[year] > 0 ) {
      easter_day = easter_dates[year];
      easter_month = 4;
    } else {
      easter_day = -easter_dates[year];
      easter_month = 3;
    }
    carnaval_finish_day = easter_day;
    carnaval_finish_month = easter_month;
    carnaval_year = year;
    add_sub_days(&carnaval_finish_day, &carnaval_finish_month, &carnaval_year, -46);
    carnaval_start_day = carnaval_finish_day;
    carnaval_start_month = carnaval_finish_month;
    add_sub_days(&carnaval_start_day, &carnaval_start_month, &carnaval_year, -5);
//    fprintf(stderr, "Carnaval runs from %d/%d to %d/%d", carnaval_start_day, carnaval_start_month, carnaval_finish_day, carnaval_finish_month);
    // Daylight savings ends 1 week later if the end date falls during Carnaval
    if( (month > carnaval_start_month  || (month == carnaval_start_month  && day >= carnaval_start_day)) &&
        (month < carnaval_finish_month || (month == carnaval_finish_month && day <  carnaval_finish_day)) )
      day += 7;
  } else if( except&EXCEPT_ROSH_HAS ) {
    unsigned char rosh_day, rosh_mon;
    rosh_day = rosh_hashanah_dates[year];
    if( rosh_day&64 ) {
      rosh_mon = 10;
      rosh_day &= 63;
    } else {
      rosh_mon = 9;
    }
    if( day == rosh_day && month == rosh_mon )
      return is_before(hour_a, day_a, hour_b, DAY_MON, week_b, orig_month, orig_year, 0);
  }


  return day_a < day || (day_a == day && hour_a < hour);
}

static unsigned char is_date_within_ramadan(unsigned char day, unsigned char month, unsigned char year) {
  unsigned char ramadan_day = 28, ramadan_month = 11, ramadan_year = 0; //28th November 2000
  int i = 0;
  while(1) {
    int duration = i < (int)sizeof(ramadan_dates) && (ramadan_dates[i]&64) ? 29 : 30;
//    fprintf(stderr, "Ramadan starts on %04d-%02d-%02d and goes for %d days.\n", ramadan_year+2000, ramadan_month, ramadan_day, duration);
    if( year < ramadan_year )
      return 0;
    if( (year == ramadan_year || year == ramadan_year-1) ) {
      unsigned char finish_day = ramadan_day, finish_month = ramadan_month, finish_year = ramadan_year;
      add_sub_days(&finish_day, &finish_month, &finish_year, duration);
      if( (year > ramadan_year || (year == ramadan_year && (month > ramadan_month || (month == ramadan_month && day >= ramadan_day)))) &&
          (year <  finish_year || (year ==  finish_year && (month <  finish_month || (month == finish_month  && day <  finish_day )))) )
        return 1;
    }

    if( i == sizeof(ramadan_dates) )
      break;
    ++ramadan_year;
    add_sub_days(&ramadan_day, &ramadan_month, &ramadan_year, -((int)(ramadan_dates[i]&63)));
    ++i;
  }
  return 0;
}

// return value is 1 for DST, 0 otherwise
static unsigned char _apply_timezone(int* time, int* date, int tz, int force_dst) {
  int seconds = *time%100;
  int minutes = (*time/100)%100;
  int hours = *time/10000;
  int year = *date%100;
  int month = (*date/100)%100;
  int day = *date/10000;
  int offset = timezones[tz_map[tz]];
  int flags = offset&0x7FFFF000L;

  if( offset < 0 ) {
    offset |= 0x7FFFF000;
    flags ^= 0x7FFFF000;
  } else {
    offset &= ~0x7FFFF000;
    flags &= 0x7FFFF000;
  }

  if( force_dst && (flags&(DST_AU|DST_NZ|DST_EU|DST_NA|DST_MX|DST_BR)) ) {
    if( flags&DST_DELTA_30 )
      offset += 70;
    else
      offset += 100;
  }

  if( offset < 0 ) {
    offset = -offset;

    minutes -= offset%100;
    if( minutes < 0 ) {
      hours -= 1;
      minutes += 60;
    }
    hours -= offset/100;
    if( hours < 0 ) {
      day -= 1;
      hours += 24;
    }
    if( day == 0 ) {
      if( --month == 0 ) {
        month += 12;
        year -= 1;
      }
      day = get_days_in_month(month, year);
    }

    offset = -offset;
  } else {
    minutes += offset%100;
    if( minutes > 60 ) {
      hours += 1;
      minutes -= 60;
    }
    hours += offset/100;
    if( hours > 24 ) {
      day += 1;
      hours -= 24;
    }
    if( day > get_days_in_month(month, year) ) {
      day = 1;
      if( ++month > 12 ) {
        month -= 12;
        year += 1;
      }
    }
  }

  if( !force_dst && flags ) {
    const daylight_savings_info* info = daylight_savings;
    const daylight_savings_info* end  = daylight_savings + (sizeof(daylight_savings)/sizeof(*daylight_savings));
    int dst_in_effect = 0;
    while( info < end ) {
      if( flags&info->flag ) {
        if( month == info->start_month ) {
          dst_in_effect = !is_before(hours, day, info->start_hour, info->start_dow, info->start_week, month, year, 0);
        } else if( month == info->finish_month ) {
          dst_in_effect = is_before(hours, day, info->finish_hour, info->finish_dow, info->finish_week, month, year, info->flag&EXCEPT_CARNAVAL);
        } else {
          if( info->start_month < info->finish_month )
            dst_in_effect = (month > info->start_month && month < info->finish_month);
          else
            dst_in_effect = (month < info->start_month || month > info->finish_month);
        }
        if( dst_in_effect && (info->flag&EXCEPT_RAMADAN) && is_date_within_ramadan(day, month, year) )
          dst_in_effect = 0;
        break;
      }
      ++info;
    }
    if( dst_in_effect )
      return _apply_timezone(time, date, tz, 1);
  }

  *time = seconds+minutes*100+hours*10000;
  *date = year+month*100+day*10000;
  return force_dst;
}

unsigned char apply_timezone(int* time, int* date, int tz) {
  return _apply_timezone(time, date, tz, 0);
}
