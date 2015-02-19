
#include <string.h>
#include <stdio.h>
#include "parsegps.h"

static int hex2bin(const char* hex) {
  int ret;
  if( hex[0] >= '0' && hex[0] <= '9' ) {
    ret = hex[0]-'0';
  } else if( hex[0] >= 'a' && hex[0] <= 'f' ) {
    ret = hex[0]-'a'+10;
  } else if( hex[0] >= 'A' && hex[0] <= 'F' ) {
    ret = hex[0]-'A'+10;
  } else {
    return -1;
  }
  ret <<= 4;
  ++hex;
  if( hex[0] >= '0' && hex[0] <= '9' ) {
    ret += hex[0]-'0';
  } else if( hex[0] >= 'a' && hex[0] <= 'f' ) {
    ret += hex[0]-'a'+10;
  } else if( hex[0] >= 'A' && hex[0] <= 'F' ) {
    ret += hex[0]-'A'+10;
  } else {
    return -1;
  }
  return ret;
}

static unsigned int CalculateNMEAChecksum(const char* buf, const char* end) {
  unsigned int ret = 0;
  while( *buf && buf != end ) {
    ret ^= (unsigned int)*((unsigned char*)buf);
    ++buf;
  }
  return ret;
}

GPSMsgType ParseGPS(const char* str, int* gps_fix, char* gps_alt, int* gps_time, int* gps_fractime, int* gps_date, int* gps_lat, int* gps_lon) {
  if( str[0] == '$' ) {
    ++str;
    const char* start = str;
    if( str[0] == 'G' && str[1] == 'P' ) {
      int i;
      char* end;

      str += 2;
      if( str[0] == 'G' && str[1] == 'G' && str[2] == 'A' && str[3] == ',' ) {
        int fixmode;
        char alt[16];

        str += 4;
        // fractional time, lat, N/S, long, E/S, 1=good fix, # sats, HDOP, altitude
        for( i = 0; i < 5; ++i ) {
          while( *str && *str != ',' )
            ++str;
          if( !*str )
            return;
          ++str;
        }

        end = 0;
        fixmode = strtoul(str, &end, 10);
        if( !end || end == str || *end != ',' )
          return;
        str = end+1;

        for( i = 0; i < 2; ++i ) {
          while( *str && *str != ',' )
            ++str;
          if( !*str )
            return;
          ++str;
        }

        end = (char*)str;
        while( *end && *end != ',' )
          ++end;
        if( end > str && end-str < 16 ) {
          strncpy(alt, str, end-str);
          alt[end-str] = '\0';

          str = end;
          while( *str && *str != '*' )
            ++str;
          if( *str != '*' || hex2bin(str+1) != CalculateNMEAChecksum(start, str) )
            return;

          *gps_fix = fixmode;
          strcpy(gps_alt, alt);
          return GPS_MSG_FIXANDALT;
//          printf("GPS fix mode = %d, altitude = %s.\n", fixmode, alt);
        }
      } else if( str[0] == 'R' && str[1] == 'M' && str[2] == 'C' && str[3] == ',' ) {
        int time, fractime, date, lat, lon, temp, temp2;

        str += 4;
        // fractional time, mode, lat, N/S, long, E/W, speed, course, date
        end = 0;
        time = strtoul(str, &end, 10);
        if( !end || end == str || *end != '.' )
          return;
        str = end+1;

        end = 0;
        fractime = strtoul(str, &end, 10);
        if( !end || end == str || *end != ',' )
          return;
        str = end+1;

        while( *str && *str != ',' )
          ++str;
        if( !*str )
          return;
        ++str;

        end = 0;
        lat = strtoul(str, &end, 10);
        if( !end || end == str || *end != '.' )
          return;
        str = end+1;
        end = 0;
        temp = strtoul(str, &end, 10);
        if( !end || end == str || *end != ',' )
          return;
        temp2 = end-str;
        while( temp2 < 2 ) {
          temp *= 10;
          ++temp2;
        }
        while( temp2 > 2 ) {
          temp /= 10;
          --temp2;
        }
        temp += (lat%100) * 100;
        lat = (lat / 100) * 10000 + (temp * 5 / 3);
        str = end+1;

        if( (*str != 'N' && *str != 'S') || str[1] != ',' )
          return;
        if( *str == 'S' )
          lat = -lat;
        str += 2;

        end = 0;
        lon = strtoul(str, &end, 10);
        if( !end || end == str || *end != '.' )
          return;
        str = end+1;
        end = 0;
        temp = strtoul(str, &end, 10);
        if( !end || end == str || *end != ',' )
          return;
        temp2 = end-str;
        while( temp2 < 2 ) {
          temp *= 10;
          ++temp2;
        }
        while( temp2 > 2 ) {
          temp /= 10;
          --temp2;
        }
        temp += (lon%100) * 100;
        lon = (lon / 100) * 10000 + (temp * 5 / 3);
        str = end+1;

        if( (*str != 'W' && *str != 'E') || str[1] != ',' )
          return;
        if( *str == 'W' )
          lon = -lon;
        str += 2;

        for( i = 0; i < 2; ++i ) {
          while( *str && *str != ',' )
            ++str;
          if( !*str )
            return;
          ++str;
        }

        end = 0;
        date = strtoul(str, &end, 10);
        if( !end || end == str || *end != ',' )
          return;

        str = end;
        while( *str && *str != '*' )
          ++str;
        if( *str != '*' || hex2bin(str+1) != CalculateNMEAChecksum(start, str) )
          return;

//        printf("time: %ld.%03ld lat: %d lon: %d date: %06ld\n", time, fractime, lat, lon, date);
        *gps_time = time;
        *gps_fractime = fractime;
        *gps_date = date;
        *gps_lat = lat;
        *gps_lon = lon;
        return GPS_MSG_TIMEDATELATLON;
      }
    }
  }
  return GPS_MSG_NONE;
}

/*
static const unsigned char month_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static unsigned long GetGPSWeek(unsigned long* pTimeOfWeek, int date, int time) {
  unsigned long day = date/10000, month = (date/100)%100, year = date%100;
  unsigned long days_since_jan_1st_1980 = 0;
  int i;
  days_since_jan_1st_1980 = (year + 20) * 365 + (year + 23) / 4 + day-6;
  for( i = 1; i < month; ++i ) {
    days_since_jan_1st_1980 += month_days[i-1];
    if( i == 2 && !(year&3) )
      days_since_jan_1st_1980 += 1;
  }
  if( pTimeOfWeek ) {
    *pTimeOfWeek = (days_since_jan_1st_1980%7)*86400 + ((time/100000)*60 + ((time/100)%100)) * 60 + (time%100);
  }
  return days_since_jan_1st_1980/7;
}

void GenerateInitialisationString(char* buf, int date, int time, int lat, int lon, const char* alt, unsigned long reset_config) {
  unsigned long Week, TimeOfWeek;
  int fraclat, fraclon;
  char* str;
  Week = GetGPSWeek(&TimeOfWeek, date, time);
  fraclat = (lat%10000)*3/5;
  lat = (lat / 10000) * 100 + fraclat / 100;
  fraclat %= 100;
  fraclon = (lon%10000)*3/5;
  lon = (lon / 10000) * 100 + fraclon / 100;
  fraclon %= 100;
  str = buf+sprintf(buf, "$PSRF104,%d.%d,%d.%d,%s,0,%ld,%ld,12,%ld", lat, fraclat, lon, fraclon, alt, TimeOfWeek, Week, reset_config);
  sprintf(str, "*%02x", CalculateNMEAChecksum(buf+1, 0));
}
*/

/* test code

int main(void) {
//  unsigned int Week, TimeOfWeek;
  char buf[256];
  ParseGPS("$GPGGA,222445.000,3345.6284,S,15116.8344,E,1,03,4.2,26.6,M,22.0,M,,0000*71");
  ParseGPS("$GPGSA,A,2,16,23,27,,,,,,,,,,4.3,4.2,1.0*30");
  ParseGPS("$GPGSV,3,1,11,23,70,114,26,09,59,208,,07,47,259,,16,44,127,36*7E");
  ParseGPS("$GPGSV,3,2,11,20,37,351,,10,27,230,,30,18,281,,27,17,076,34*78");
  ParseGPS("$GPGSV,3,3,11,19,09,049,,06,04,269,,32,03,022,*48");
  ParseGPS("$GPRMC,222445.000,A,3345.6284,S,15116.8344,E,0.00,233.67,081214,,,A*79");
  ParseGPS("$GPVTG,233.67,T,,M,0.00,N,0.0,K,N*01");
  ParseGPS("$GPGGA,222446.000,3345.6284,S,15116.8344,E,1,03,4.2,26.6,M,22.0,M,,0000*72");
  ParseGPS("$GPRMC,222446.000,A,3345.6284,S,15116.8344,E,0.00,233.67,081214,,,A*7A");
  ParseGPS("$GPVTG,233.67,T,,M,0.00,N,0.0,K,N*01");
  ParseGPS("$GPGGA,222447.000,3345.6284,S,15116.8344,E,1,03,4.1,26.6,M,22.0,M,,0000*70");
  ParseGPS("$GPRMC,222447.000,A,3345.6284,S,15116.8344,E,0.00,233.67,081214,,,A*7B");
  ParseGPS("$GPVTG,233.67,T,,M,0.00,N,0.0,K,N*01");
  ParseGPS("$GPGGA,222448.000,3345.6284,S,15116.8344,E,1,03,4.1,26.6,M,22.0,M,,0000*7F");
  ParseGPS("$GPRMC,222448.000,A,3345.6284,S,15116.8344,E,0.00,233.67,081214,,,A*74");
  ParseGPS("$GPVTG,233.67,T,,M,0.00,N,0.0,K,N*01");
  ParseGPS("$GPGGA,004156.000,3345.6130,S,15116.8031,E,1,05,3.9,13.2,M,22.0,M,,0000*75");
  ParseGPS("$GPRMC,004156.000,A,3345.6130,S,15116.8031,E,0.00,270.43,091214,,,A*75");
  
  GenerateInitialisationString(buf, 1);
  printf("%s\n", buf);
//  Week = GetGPSWeek(&TimeOfWeek);
//  printf("GPS Week: %d Time of week: %d\n", Week, TimeOfWeek);

  return 0;
}

*/
