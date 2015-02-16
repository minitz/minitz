
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "timezones.h"
#include "tzmaths.h"

#define INCLUDE_TZ_COORDS
#ifdef TEST_MODE
#define INCLUDE_TZ_NAMES
#endif
#include "tz_coords.h"


typedef struct {
  int tz, lat, lon;
  const unsigned char* ptr;
} tz_state;

static tz_state cur_tz, saved_tz;


static void init_tz(tz_state* pState) {
  pState->tz = 0;
  pState->lat = 0;
  pState->lon = 0;
  pState->ptr = tz_coords;
}
static void save_tz_state() {
  memcpy(&saved_tz, &cur_tz, sizeof(saved_tz));
}
static void restore_tz_state() {
  memcpy(&cur_tz, &saved_tz, sizeof(cur_tz));
}

static int process_tz_token(tz_state* pState) {
  if( pState->ptr == tz_coords + sizeof(tz_coords) )
    return -1;

  if( (pState->ptr[0]&3) != 3 ) {
    unsigned short val = pState->ptr[0] + (((unsigned short)pState->ptr[1])<<8);
    val = (((int)val+1)*3)>>2;
    if( val >= 40960 ) {
      unsigned long val2 = (((unsigned long)(val-40960))<<8)|pState->ptr[2];
      int dlat = (int)(val2 / 1447) - 723;
      int dlon = (int)(val2 % 1447) - 723;
      pState->lat += dlat;
      pState->lon += dlon;
      pState->ptr += 3;
    } else {
      int dlat = (int)(val / 201) - 100;
      int dlon = (int)(val % 201) - 100;
      pState->lat += dlat;
      pState->lon += dlon;
      pState->ptr += 2;
    }
    return 0;
  } else if( !(pState->ptr[0]&4) ) {
    unsigned long val = (((unsigned long)pState->ptr[0])>>3) + (((unsigned long)pState->ptr[1])<<5) + (((unsigned long)pState->ptr[2])<<13) + (((unsigned long)pState->ptr[3])<<21);
    int dlat = (int)(val / 20001) - 10000;
    int dlon = (int)(val % 20001) - 10000;
    pState->lat += dlat;
    pState->lon += dlon;
    pState->ptr += 4;
    return 0;
  } else if( !(pState->ptr[0]&8) ) {
    unsigned long val1 = (((unsigned long)pState->ptr[0])>>4) + (((unsigned long)pState->ptr[1])<<4) + (((unsigned long)pState->ptr[2])<<12) + (((unsigned long)(pState->ptr[3]&3))<<20);
    unsigned long val2 = (((unsigned long)pState->ptr[3])>>2) + (((unsigned long)pState->ptr[4])<<6) + (((unsigned long)pState->ptr[5])<<14);
    int dlat = (int)val2 - 1800000;
    int dlon = (int)val1 - 1800000;
    pState->lat += dlat;
    pState->lon += dlon;
    pState->ptr += 6;
    return 0;
  } else if( !(pState->ptr[0]&16) ) {
    pState->lat = 0;
    pState->lon = 0;
    pState->ptr += 1;
    return 1;
  } else {
    pState->lat = 0;
    pState->lon = 0;
    pState->tz = pState->ptr[1];
    pState->ptr += 2;
    return 1;
  }
}

static int do_lines_intersect(long p0_x, long p0_y, long p1_x, long p1_y, long p2_x, long p2_y, long p3_x, long p3_y) {
    long s02_x, s02_y, s10_x, s10_y, s32_x, s32_y;
    long long s_numer, t_numer, denom;
    s10_x = p1_x - p0_x;
    s10_y = p1_y - p0_y;
    s32_x = p3_x - p2_x;
    s32_y = p3_y - p2_y;

    denom = (long long)s10_x * (long long)s32_y - (long long)s32_x * (long long)s10_y;
    if (denom == 0)
        return 0; // Collinear
    char denomPositive = denom > 0;

    s02_x = p0_x - p2_x;
    s02_y = p0_y - p2_y;
    s_numer = (long long)s10_x * (long long)s02_y - (long long)s10_y * (long long)s02_x;
    if ((s_numer < 0) == denomPositive)
        return 0; // No collision

    t_numer = (long long)s32_x * (long long)s02_y - (long long)s32_y * (long long)s02_x;
    if ((t_numer < 0) == denomPositive)
        return 0; // No collision

    if (((s_numer > denom) == denomPositive) || ((t_numer > denom) == denomPositive))
        return 0; // No collision

    // experimental, prevent problems when line intersects polygon at vertex
    if( t_numer == 0 )
      return 0;

    return 1;
}

static int g_search_lat, g_search_lon, g_nearest_dist, g_nearest_tz, g_tz_found;
static int g_minlat, g_minlon, g_maxlat, g_maxlon, g_avglat, g_avglon, g_randlat, g_randlon, g_state, g_search_tz;
static int g_first, g_firstlat, g_firstlon, g_lastlat, g_lastlon, g_num_intersections;

static void init_get_timezone_extents() {
  g_minlat = 1800000;
  g_minlon = 1800000;
  g_maxlat = -1800000;
  g_maxlon = -1800000;
  save_tz_state();
}
static int continue_get_timezone_extents(int max_calcs) {
  while( max_calcs && !process_tz_token(&cur_tz) ) {
    --max_calcs;
    if( cur_tz.lat < g_minlat )
      g_minlat = cur_tz.lat;
    if( cur_tz.lat > g_maxlat )
      g_maxlat = cur_tz.lat;
    if( cur_tz.lon < g_minlon )
      g_minlon = cur_tz.lon;
    if( cur_tz.lon > g_maxlon )
      g_maxlon = cur_tz.lon;
  }
  return max_calcs;
}
static void init_is_point_within_cur_timezone() {
  g_first = 1;
  g_num_intersections = 0;
}
static int continue_is_point_within_cur_timezone(int max_calcs) {
  int distlat, distlon;

  while( max_calcs && !process_tz_token(&cur_tz) ) {
    --max_calcs;
    if( g_first ) {
      g_firstlat = cur_tz.lat;
      g_firstlon = cur_tz.lon;
      g_search_tz = cur_tz.tz;
    } else {
      if( do_lines_intersect(g_lastlat, g_lastlon, cur_tz.lat, cur_tz.lon, g_search_lat, g_search_lon, g_randlat, g_randlon) )
        ++g_num_intersections;
    }
    g_first = 0;
    g_lastlat = cur_tz.lat;
    g_lastlon = cur_tz.lon;

    distlat = cur_tz.lat - g_search_lat;
    if( distlat < 0 )
      distlat = -distlat;
    distlon = cur_tz.lon - g_search_lon;
    if( distlon < 0 )
      distlon = -distlon;
    if( distlat < 10000 && distlon < 10000 ) {
      int distsq = distlat * distlat + distlon * distlon;
      if( distsq < g_nearest_dist ) {
        g_nearest_dist = distsq;
        g_nearest_tz = g_search_tz;
      }
    }
  }
  if( max_calcs )
    if( do_lines_intersect(g_firstlat, g_firstlon, g_lastlat, g_lastlon, g_search_lat, g_search_lon, g_randlat, g_randlon) )
      ++g_num_intersections;

  return max_calcs;
}

void init_get_timezone(int lat, int lon) {
  g_search_lat = lat;
  g_search_lon = lon;
  g_nearest_dist = 100000000;
  g_nearest_tz = -1;
  g_tz_found = -1;
  g_state = 0;
  init_tz(&cur_tz);
}
int continue_get_timezone(int max_calcs) {
  if( g_tz_found != -1 )
    return g_tz_found;
  while( g_tz_found == -1 && max_calcs && (cur_tz.ptr != tz_coords + sizeof(tz_coords) || g_state < 3) ) {
    --max_calcs;
    if( g_state == 0 ) {
      init_get_timezone_extents();
      g_state = 1;
    }
    if( g_state == 1 ) {
      if( !max_calcs )
        return -1;
      max_calcs = continue_get_timezone_extents(max_calcs);
      if( max_calcs || cur_tz.ptr == tz_coords + sizeof(tz_coords) ) {
        if( (g_search_lat >= g_minlat && g_search_lat <= g_maxlat &&
            g_search_lon >= g_minlon && g_search_lon <= g_maxlon) || 0 ) {
          restore_tz_state();
          g_avglat = (g_minlat + g_maxlat) / 2;
          g_avglon = (g_minlon + g_maxlon) / 2;
          g_state = 2;
        } else {
          if( cur_tz.ptr != tz_coords + sizeof(tz_coords) ) {
            g_state = 0;
            continue;
          } else {
            return -2;
          }
        }
      }
    }
    if( g_state == 2 ) {
      if( !max_calcs )
        return -1;

      int randvallat = rand()&2047;
      int randvallon = rand()&2047;
      if( randvallat < 1024 ) {
        g_randlat = g_minlat - (g_maxlat - g_minlat) * (randvallat + 1024) / 1024;
      } else {
        g_randlat = g_maxlat + (g_maxlat - g_minlat) * (randvallat       ) / 1024;
      }
      if( randvallon < 1024 ) {
        g_randlon = g_minlon - (g_maxlon - g_minlon) * (randvallon + 1024) / 1024;
      } else {
        g_randlon = g_maxlon + (g_maxlon - g_minlon) * (randvallon       ) / 1024;
      }
      --max_calcs;
      init_is_point_within_cur_timezone();
      g_state = 3;
    }
    if( g_state == 3 ) {
      if( !max_calcs )
        return -1;

      // need to handle case where polygon point is on line
      max_calcs = continue_is_point_within_cur_timezone(max_calcs);
      if( max_calcs || cur_tz.ptr == tz_coords + sizeof(tz_coords) ) {
        if( g_num_intersections&1 ) {
          // found the current timezone
          g_tz_found = g_search_tz;
          return g_tz_found;
        } else {
          // check next time zone for a match
          g_state = 0;
          continue;
        }
      }
    }
  }

  if( cur_tz.ptr == tz_coords + sizeof(tz_coords) ) {
    if( g_state == 3 && g_nearest_tz == -1 )
      return -2;
    else
      return g_nearest_tz;
  } else {
    return -1;
  }
}


#ifdef TEST_MODE
/* test code */

int get_timezone(long lat, long lon) {
  int tz;
  init_get_timezone(lat, lon);
  while( (tz = continue_get_timezone(10)) == -1 )
    ;
  return tz;
}

void print_timezone_info(const char* name, long lat, long lon, int expected_tz) {
  int tz = get_timezone(lat, lon);
  if( tz < 0 ) {
    printf("Timezone not found for %s!\n", name);
    exit(-1);
  } else {
    time_t now = time(NULL);
    struct tm* gmt = gmtime(&now);
    int tim = gmt->tm_sec + gmt->tm_min*100 + gmt->tm_hour*10000;
    int dat = (gmt->tm_year-100) + (gmt->tm_mon+1)*100 + gmt->tm_mday*10000;
    int using_dst = apply_timezone(&tim, &dat, tz);
    printf("%s is in %s (%d) timezone, local time is %02d:%02d:%02d%s on %02d/%02d/%02d\n", name, tz_names[tz], tz, tim/10000, (tim/100)%100, tim%100, using_dst ? "*" : "", dat/10000, (dat/100)%100, dat%100);
    if( tz != expected_tz ) {
      printf("Expected timezone = %d. Aborting test.\n", expected_tz);
      exit(-1);
    }
  }
}

int main(void) {
  int expected_tz = 0;

  srand(time(NULL));
  print_timezone_info("Sydney", -338600, 1512094, expected_tz);
  print_timezone_info("Albury", -360806, 1469158, expected_tz);
  print_timezone_info("Melbourne", -378136, 1449631, expected_tz);
  print_timezone_info("Wodonga", -361214, 1468881, expected_tz);
  print_timezone_info("Hobart", -428806, 1473250, expected_tz);
  print_timezone_info("Currie", -399311, 1438506, expected_tz);
  print_timezone_info("Bruny Island", -433840, 1472900, expected_tz);
  ++expected_tz;
  print_timezone_info("Brisbane", -274679, 1530278, expected_tz);
  print_timezone_info("Lindeman", -204500, 1490333, expected_tz);
  print_timezone_info("Mornington Island", -165372, 1394026, expected_tz);
  ++expected_tz;
  print_timezone_info("Adelaide", -349290, 1386010, expected_tz);
  print_timezone_info("Broken Hill", -319567, 1414678, expected_tz);
  print_timezone_info("Kangaroo Island", -357755, 1372336, expected_tz);
  ++expected_tz;
  print_timezone_info("Darwin", -124500, 1308333, expected_tz);
  print_timezone_info("Groote Eylandt", -140000, 1363600, expected_tz);
  print_timezone_info("Tiwi Island", -113600, 1310000, expected_tz);
  ++expected_tz;
  print_timezone_info("Perth", -319522, 1158589, expected_tz);
  ++expected_tz;
  print_timezone_info("Eucla", -316750, 1288831, expected_tz);
  ++expected_tz;
  print_timezone_info("Lord Howe", -315500, 1590833, expected_tz);
  ++expected_tz;
  print_timezone_info("Bantam Village", -121177, 968969, expected_tz);
  ++expected_tz;
  print_timezone_info("Macquarie Island", -544930, 1589423, expected_tz);
  ++expected_tz;
  print_timezone_info("Auckland", -368404, 1747399, expected_tz);
  print_timezone_info("Christchurch", -435300, 1726203, expected_tz);
  ++expected_tz;
  print_timezone_info("Chatham", -440333, -1764333, expected_tz);
  ++expected_tz;
  print_timezone_info("Port Moresby", 95136, 1472188, expected_tz);
  print_timezone_info("Guam", 135000, 1448000, expected_tz);
  ++expected_tz;
  print_timezone_info("Sorong", -8667, 1312500, expected_tz);
  ++expected_tz;
  print_timezone_info("Malila", 145833, 1209667, expected_tz);
  print_timezone_info("Makassar", -51333, 1194167, expected_tz);
  ++expected_tz;
  print_timezone_info("New Caledonia", -212500, 1653000, expected_tz);
  print_timezone_info("Solomon Islands", -94667, 1598167, expected_tz);
  ++expected_tz;
  print_timezone_info("Suva", -181416, 1784419, expected_tz);
  ++expected_tz;
  print_timezone_info("Honolulu", 213000, -1578167, expected_tz);
  ++expected_tz;
  print_timezone_info("Anchorage", 612167, -1499000, expected_tz);
  ++expected_tz;
  print_timezone_info("Los Angeles", 340500, -1182500, expected_tz);
  ++expected_tz;
  print_timezone_info("Jeddito", 357719, -1101269, expected_tz);
  ++expected_tz;
  print_timezone_info("Flagstaff", 351992, -1116311, expected_tz);
  print_timezone_info("Phoenix", 334500, -1120677, expected_tz);
  ++expected_tz;
  print_timezone_info("Denver", 397392, -1049847, expected_tz);
  print_timezone_info("Kayenta", 367142, -1102603, expected_tz);
  ++expected_tz;
  print_timezone_info("Chicago", 418369, -876847, expected_tz);
  ++expected_tz;
  print_timezone_info("New York", 407127, -740059, expected_tz);
  print_timezone_info("Guantanamo Bay", 199000, -751500, expected_tz);
  ++expected_tz;
  print_timezone_info("Pago Pago", -142794, -1707006, expected_tz);
  ++expected_tz;
  print_timezone_info("Blanc-Sablon", 514167, -571333, expected_tz);
  ++expected_tz;
  print_timezone_info("St. John's", 475675, -527072, expected_tz);
  ++expected_tz;
  print_timezone_info("Halifax", 446478, -635714, expected_tz);
  print_timezone_info("Fredericton", 459500, -666667, expected_tz);
  print_timezone_info("Churchill Falls", 535710, -642786, expected_tz);
  ++expected_tz;
  print_timezone_info("Coral Harbour", 641369, -831642, expected_tz);
  ++expected_tz;
  print_timezone_info("Montreal", 455000, -735667, expected_tz);
  print_timezone_info("Ottawa", 454214, -756919, expected_tz);
  print_timezone_info("Toronto", 437000, -794000, expected_tz);
  print_timezone_info("Thunder Bay", 483832, -892461, expected_tz);
  ++expected_tz;
  print_timezone_info("Dawson Creek", 557606, -1202356, expected_tz);
  ++expected_tz;
  print_timezone_info("Regina", 504547, -1046067, expected_tz);
  ++expected_tz;
  print_timezone_info("Winnipeg", 498994, -971392, expected_tz);
  print_timezone_info("Kenora", 498387, -928123, expected_tz);
  ++expected_tz;
  print_timezone_info("Vancouver", 492500, -1231000, expected_tz);
  print_timezone_info("Whitehorse", 607167, -1350500, expected_tz);
  ++expected_tz;
  print_timezone_info("Calgary", 510500, -1140667, expected_tz);
  print_timezone_info("Edmonton", 535333, -1135000, expected_tz);
  print_timezone_info("Yellowknife", 624422, -1143975, expected_tz);
  ++expected_tz;
  print_timezone_info("Paris", 488567, 23508, expected_tz);
  print_timezone_info("Berlin", 525167, 133833, expected_tz);
  print_timezone_info("Krakow", 500614, 199372, expected_tz);
  ++expected_tz;
  print_timezone_info("London", 515072, -1275, expected_tz);
  print_timezone_info("Lisbon", 387139, -91394, expected_tz);
  print_timezone_info("Reykjavic", 641333, -219333, expected_tz);
  print_timezone_info("Torshavn", 620117, -67675, expected_tz);
  ++expected_tz;
  print_timezone_info("Ankara", 399300, 328600, expected_tz);
  print_timezone_info("Kiev", 504500, 305233, expected_tz);
  print_timezone_info("Sofia", 427000, 233333, expected_tz);
  print_timezone_info("Akrotiri", 345833, 329833, expected_tz);
  ++expected_tz;
  print_timezone_info("Moscow", 557500, 376167, expected_tz);
  print_timezone_info("Minsk", 539000, 275667, expected_tz);
  print_timezone_info("Sevastopol", 445633, 335167, expected_tz);
  print_timezone_info("Vorkuta", 675000, 640333, expected_tz);
  ++expected_tz;
  print_timezone_info("Shanghai", 312000, 1215000, expected_tz);
  print_timezone_info("Shenzhen", 225500, 1141000, expected_tz);
  print_timezone_info("Taipei", 250333, 1216333, expected_tz);
  ++expected_tz;
  print_timezone_info("Tokyo", 356895, 1396917, expected_tz);
  print_timezone_info("Seoul", 375667, 1269781, expected_tz);
  print_timezone_info("Pyongyang", 390194, 1257381, expected_tz);
  ++expected_tz;
  print_timezone_info("Dhaka", 237000, 903750, expected_tz);
  ++expected_tz;
  print_timezone_info("Thimphu", 274667, 896417, expected_tz);
  ++expected_tz;
  print_timezone_info("Kathmandu", 277000, 853333, expected_tz);
  ++expected_tz;
  print_timezone_info("Male", 41753, 735089, expected_tz);
  ++expected_tz;
  print_timezone_info("Diego Garcia", -73133, 724111, expected_tz);
  ++expected_tz;
  print_timezone_info("Bangkok", 137500, 1004667, expected_tz);
  print_timezone_info("Bandung", -69147, 1076098, expected_tz);
  print_timezone_info("Surabaya", -72653, 1127425, expected_tz);
  print_timezone_info("Jakarta", -62000, 1068167, expected_tz);
  ++expected_tz;
  print_timezone_info("Rangoon", 168000, 961500, expected_tz);
  ++expected_tz;
  print_timezone_info("Rabat", 340209, -68416, expected_tz);
  print_timezone_info("El Aaiun", 271536, -132033, expected_tz);
  ++expected_tz;
  print_timezone_info("Accra", 55500, -2000, expected_tz);
  print_timezone_info("Bamako", 126500, -80000, expected_tz);
  print_timezone_info("Banjul", 134531, -165775, expected_tz);
  print_timezone_info("Bissau",118500, -155667, expected_tz);
  print_timezone_info("Conakry", 95092, -137122, expected_tz);
  print_timezone_info("Dakar", 146928, -174467, expected_tz);
  print_timezone_info("Freetown", 84844, -132344, expected_tz);
  print_timezone_info("Lome", 61319, 12228, expected_tz);
  print_timezone_info("Nouakchott", 181000, -159500, expected_tz);
  print_timezone_info("Monrovia", 63133, -108014, expected_tz);
  print_timezone_info("Ouagadougou", 123572, -15353, expected_tz);
  print_timezone_info("Yamoussoukro", 68167, -52833, expected_tz);
  ++expected_tz;
  print_timezone_info("Algiers", 367667, 32167, expected_tz);
  print_timezone_info("Tunis", 340000, 90000, expected_tz);
  ++expected_tz;
  print_timezone_info("Tripoli", 329022, 131858, expected_tz);
  ++expected_tz;
  print_timezone_info("Cairo", 300500, 312333, expected_tz);
  ++expected_tz;
  print_timezone_info("Windhoek", -225700, 170836, expected_tz);
  ++expected_tz;
  print_timezone_info("Abuja", 90667, 74833, expected_tz);
  print_timezone_info("Bangui", 43667, 185833, expected_tz);
  print_timezone_info("Brazzaville", -42678, 152919, expected_tz);
  print_timezone_info("Cotonou", 63667, 24333, expected_tz);
  print_timezone_info("Kinshasa", -43250, 153222, expected_tz);
  print_timezone_info("Libreville", 3901, 94544, expected_tz);
  print_timezone_info("Luanda", -88383, 132344, expected_tz);
  print_timezone_info("Malabo", 37500, 87833, expected_tz);
  print_timezone_info("Niamey", 135214, 21053, expected_tz);
  print_timezone_info("N'Djamena", 121131, 150492, expected_tz);
  print_timezone_info("Porto-Novo", 64972, 26050, expected_tz);
  print_timezone_info("Yaounde", 38667, 115167, expected_tz);
  ++expected_tz;
  print_timezone_info("Addis Ababa", 90300, 387400, expected_tz);
  print_timezone_info("Antananarivo", -189333, 475167, expected_tz);
  print_timezone_info("Asmara", 153333, 389333, expected_tz);
  print_timezone_info("Dar es Salaam", -68000, 392833, expected_tz);
  print_timezone_info("Djibouti City", 116000, 431667, expected_tz);
  print_timezone_info("Juba", 48500, 316000, expected_tz);
  print_timezone_info("Kampala", 3136, 325811, expected_tz);
  print_timezone_info("Khartoum", 156333, 325333, expected_tz);
  print_timezone_info("Mogadisu", 20333, 453500, expected_tz);
  print_timezone_info("Moroni", -117500, 432000, expected_tz);
  print_timezone_info("Nairobi", -12833, 368167, expected_tz);
  ++expected_tz;
  print_timezone_info("Doha", 252867, 515333, expected_tz);
  print_timezone_info("Dubai", 249500, 553333, expected_tz);
  print_timezone_info("Muscat", 236100, 585400, expected_tz);
  ++expected_tz;
  print_timezone_info("Cape Town", -339253, 184239, expected_tz);
  print_timezone_info("Bloemfontein", -291167, 262167, expected_tz);
  print_timezone_info("Lobamba", -264167, 311667, expected_tz);
  print_timezone_info("Maseru", -293100, 274800, expected_tz);
  print_timezone_info("Mbabane", -263167, 311333, expected_tz);
  print_timezone_info("Pretoria", -257461, 281881, expected_tz);
//  ++expected_tz;
  print_timezone_info("Bujumbura", -33833, 293667, expected_tz);
  print_timezone_info("Gaborone", -246581, 259122, expected_tz);
  print_timezone_info("Harare", -178639, 310297, expected_tz);
  print_timezone_info("Kindu", -29500, 259500, expected_tz);
  print_timezone_info("Kigali", -19439, 300594, expected_tz);
  print_timezone_info("Lilongwe", -139833, 337833, expected_tz);
  print_timezone_info("Lusaka", -154167, 282833, expected_tz);
  print_timezone_info("Maputo", -259667, 325833, expected_tz);
  ++expected_tz;
  print_timezone_info("Praia", 149180, -235090, expected_tz);
  ++expected_tz;
  print_timezone_info("Jamestown", -159244, -57181, expected_tz);
  ++expected_tz;
  print_timezone_info("Port Louis", -201644, 575041, expected_tz);
  print_timezone_info("Victoria", -46167, 554500, expected_tz);
  ++expected_tz;
  print_timezone_info("Tehran", 356961, 514231, expected_tz);
  ++expected_tz;
  print_timezone_info("Kabul", 345333, 691667, expected_tz);
  ++expected_tz;
  print_timezone_info("Jerusalem", 317833, 352167, expected_tz);
  ++expected_tz;
  print_timezone_info("Neve Dekalim", 313567, 342750, expected_tz);
  ++expected_tz;
  print_timezone_info("Ramallah", 319000, 352000, expected_tz);
  ++expected_tz;
  print_timezone_info("Damascus", 335130, 362920, expected_tz);
  print_timezone_info("Amman", 319333, 359333, expected_tz);
  print_timezone_info("Beirut", 338869, 355131, expected_tz);
  ++expected_tz;
  print_timezone_info("Sao Paolo", -235500, -466333, expected_tz);
  print_timezone_info("Rio de Janeiro", -229000, -432000, expected_tz);
  print_timezone_info("Salvador", -129747, -384767, expected_tz);
  print_timezone_info("Belo Horizonte", -199167, -439333, expected_tz);
  print_timezone_info("Brasilia", -157939, -478828, expected_tz);
  print_timezone_info("Curitiba", -254167, -492500, expected_tz);
  print_timezone_info("Porto Alegre", -300331, -512300, expected_tz);
  print_timezone_info("Guarulhos", -234667, -465333, expected_tz);
  print_timezone_info("Goiania", -166667, -492500, expected_tz);
  ++expected_tz;
  print_timezone_info("Fortaleza", -37183, -385428, expected_tz);
  print_timezone_info("Recife", -80500, -349000, expected_tz);
  print_timezone_info("Belem", -14558, -485039, expected_tz);
  ++expected_tz;
  print_timezone_info("Asuncion", -252822, -567351, expected_tz);
  ++expected_tz;
  print_timezone_info("Cuiaba", -155958, -560969, expected_tz);
  print_timezone_info("Campo Grande", -204622, -546111, expected_tz);
  ++expected_tz;
  print_timezone_info("Montevideo", -348836, -561819, expected_tz);
  ++expected_tz;
  print_timezone_info("Buenos Aires", -346033, -583817, expected_tz);
  ++expected_tz;
  print_timezone_info("Caracas", 105000, -669167, expected_tz);
  ++expected_tz;
  print_timezone_info("Lima", -120433, -770283, expected_tz);
  print_timezone_info("Quito", -2333, -785167, expected_tz);
  print_timezone_info("Bogota", 45981, -740758, expected_tz);
  ++expected_tz;
  print_timezone_info("Santiago", -334500, -706667, expected_tz);
  print_timezone_info("Sucre", -190500, -652500, expected_tz);
  print_timezone_info("Manaus", -31000, -600167, expected_tz);
  ++expected_tz;
  print_timezone_info("Port Stanley", -516921, -578589, expected_tz);
  ++expected_tz;
  print_timezone_info("Puerto Baquerizo Moreno", -9025, -896092, expected_tz);
  ++expected_tz;
  print_timezone_info("Panama City", 89833, -795167, expected_tz);
  ++expected_tz;
  print_timezone_info("Tegucigalpa", 140833, -872167, expected_tz);
  print_timezone_info("Guatemala City", 146133, -905353, expected_tz);
  print_timezone_info("Belmopan", 172514, -887669, expected_tz);
  print_timezone_info("San Salvadore", 136900, -891900, expected_tz);
  print_timezone_info("Managua", 121364, -862514, expected_tz);
  print_timezone_info("San Jose, Costa Rica", 96000, -839500, expected_tz);
  ++expected_tz;
  print_timezone_info("Tijuana", 325250, -1170333, expected_tz);
  ++expected_tz;
  print_timezone_info("La Paz", 241422, -1103108, expected_tz);
  print_timezone_info("Nogales", 313186, -1109458, expected_tz);
  print_timezone_info("Chihuahua", 286333, -1060833, expected_tz);
  ++expected_tz;
  print_timezone_info("Merida", 209700, -896200, expected_tz);
  ++expected_tz;
  print_timezone_info("Cancun", 211606, -868475, expected_tz);
  print_timezone_info("Monterrey", 256667, -1003000, expected_tz);
  print_timezone_info("Mexico City", 194333, -991333, expected_tz);
  print_timezone_info("Acapulco", 168636, -998825, expected_tz);
  print_timezone_info("Veracruz", 194333, -963833, expected_tz);
  ++expected_tz;
  print_timezone_info("Havana", 231333, -823833, expected_tz);
  print_timezone_info("Nassau", 250600, -773450, expected_tz);
  ++expected_tz;
  print_timezone_info("Port-au-Prince", 185333, -723333, expected_tz);
  ++expected_tz;
  print_timezone_info("Kingston", 179833, -768000, expected_tz);
  print_timezone_info("George Town", 193034, -813863, expected_tz);
  ++expected_tz;
  print_timezone_info("Santo Domingo", 184667, -699500, expected_tz);
  print_timezone_info("San Juan", 184500, -660667, expected_tz);
  print_timezone_info("Kingstown", 131578, -612250, expected_tz);
  ++expected_tz;
  print_timezone_info("Samara", 532028, 501408, expected_tz);
  print_timezone_info("Izhevsk", 568333, 531833, expected_tz);
  ++expected_tz;
  print_timezone_info("Orenburg", 517833, 551000, expected_tz);
  print_timezone_info("Salekhard", 665333, 666000, expected_tz);
  print_timezone_info("Surgut", 612500, 734333, expected_tz);
  ++expected_tz;
  print_timezone_info("Omsk", 549833, 733667, expected_tz);
  print_timezone_info("Tomsk", 565000, 849667, expected_tz);
  ++expected_tz;
  print_timezone_info("Khatanga", 719797, 1024728, expected_tz);
  print_timezone_info("Kyzyl", 517000, 944500, expected_tz);
//  print_timezone_info("Kemerovo", 553608, 860889, expected_tz);
  print_timezone_info("Vanavara", 603333, 1022667, expected_tz);
  ++expected_tz;
  print_timezone_info("Bratsk", 561167, 1016000, expected_tz);
  print_timezone_info("Bodaybo", 578667, 1142000, expected_tz);
  print_timezone_info("Irkutsk", 523122, 1042958, expected_tz);
  ++expected_tz;
  print_timezone_info("Pevek", 697000, 1702833, expected_tz);
  print_timezone_info("Uelen", 661594, 1698092, expected_tz);
  print_timezone_info("Anadyr", 647333, 1775167, expected_tz);
  print_timezone_info("Palana", 590833, 1599333, expected_tz);
  print_timezone_info("Petropavlovsk-Kamchatsky", 530167, 1586500, expected_tz);
  ++expected_tz;
  print_timezone_info("Yakutsk", 620333, 1297333, expected_tz);
  print_timezone_info("Tynda", 551667, 1247167, expected_tz);
  print_timezone_info("Tiksi", 716377, 1288765, expected_tz);
  ++expected_tz;
  print_timezone_info("Khasan", 424283, 1306456, expected_tz);
  print_timezone_info("Birobidzhan", 488000, 1329333, expected_tz);
  print_timezone_info("Okha", 535833, 1429333, expected_tz);
  print_timezone_info("Ohkotsk", 594000, 1432000, expected_tz);
  print_timezone_info("Magadan", 595666, 1508000, expected_tz);
  print_timezone_info("Evensk", 619244, 1592300, expected_tz);
  ++expected_tz;
  print_timezone_info("Verkhoyansk", 675500, 1333833, expected_tz);
  ++expected_tz;
  print_timezone_info("Druzhina", 682000, 1453400, expected_tz);
  print_timezone_info("Chersky", 687667, 1613333, expected_tz);
  ++expected_tz;
  print_timezone_info("Tbilisi", 417167, 447833, expected_tz);
  print_timezone_info("Yerevan", 401833, 445167, expected_tz);
  print_timezone_info("Baku", 403953, 498822, expected_tz);
  ++expected_tz;
  print_timezone_info("Bishkek", 428747, 746122, expected_tz);
  ++expected_tz;
  print_timezone_info("Tashkent", 412667, 692167, expected_tz);
  ++expected_tz;
  print_timezone_info("Itangar", 271000, 936200, expected_tz);
  print_timezone_info("Mumbai", 189750, 728258, expected_tz);
  print_timezone_info("New Delhi", 286139, 772089, expected_tz);
  print_timezone_info("Kotte", 69108, 798878, expected_tz);
  ++expected_tz;
  print_timezone_info("Olgiy", 490000, 922408, expected_tz);
  ++expected_tz;
  print_timezone_info("Ulan Bator", 479200, 1069200, expected_tz);
  ++expected_tz;
  print_timezone_info("Qaanaaq", 774667, -692306, expected_tz);
  ++expected_tz;
  print_timezone_info("Ittoqqortoormiit", 704853, -219667, expected_tz);
  ++expected_tz;
  print_timezone_info("Danmarkshavn", 767667, -186667, expected_tz);
  ++expected_tz;
  print_timezone_info("Nuuk", 641750, -517389, expected_tz);
  ++expected_tz;
  print_timezone_info("Longyearbyen", 782200, 156500, expected_tz);
  ++expected_tz;
  print_timezone_info("Prince George Land", 804453, 490125, expected_tz);
  ++expected_tz;
  print_timezone_info("Port-aux-Francais", -493500, 702167, expected_tz);
  ++expected_tz;
  print_timezone_info("Edinburgh of the Seven Seas", -370667, -123167, expected_tz);
  ++expected_tz;
  print_timezone_info("King Edward Point", -542833, -365000, expected_tz);
  ++expected_tz;
  print_timezone_info("Marion Island", -467731, 378525, expected_tz);
  ++expected_tz;
  print_timezone_info("Ile aux Cochons", -461000, 502333, expected_tz);
  print_timezone_info("Ile de la Possession", -464000, 517667, expected_tz);
  ++expected_tz;
  print_timezone_info("Apia", -138333, -1717500, expected_tz);
  ++expected_tz;
  print_timezone_info("Nuku'alofa", -211333, -1752000, expected_tz);
  print_timezone_info("Nukunonu", -91683, -1718097, expected_tz);
  ++expected_tz;
  print_timezone_info("Atuona", -98030, -1390396, expected_tz);
  ++expected_tz;
  print_timezone_info("Papeete", -175350, -1495696, expected_tz);
  print_timezone_info("Nuku Hiva", -88667, -1401333, expected_tz);
  ++expected_tz;
  print_timezone_info("Adamstown", -254000, -1304000, expected_tz);
  ++expected_tz;

  if( expected_tz == sizeof(tz_map) )
    printf("%d timezones tested, all passed.\n", expected_tz);
  else
    printf("%d timezones tested, all passed, %d untested.\n", expected_tz, (int)(sizeof(tz_map) - expected_tz));

  return 0;
}
#endif
