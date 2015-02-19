
#ifndef _PARSEGPS_H_
#define _PARSEGPS_H_

typedef enum { GPS_MSG_NONE, GPS_MSG_FIXANDALT, GPS_MSG_TIMEDATELATLON } GPSMsgType;

// gps_alt should be at least 16 characters long
GPSMsgType ParseGPS(const char* str, int* gps_fix, char* gps_alt, int* gps_time, int* gps_fractime, int* gps_date, int* gps_lat, int* gps_lon);
// buf should be at least 100 characters long
//void GenerateInitialisationString(char* buf, int date, int time, int lat, int lon, const char* alt, unsigned long reset_config);

#endif//_PARSEGPS_H_
