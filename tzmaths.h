
/* minitz - MINiaturised International TimeZone database v1.0
   
   Written by Nicholas Vinen. Please see README and LICENSE files.
   
   Time zone offset / daylight savings manipulation header. */

#ifndef TZMATHS_H
#define	TZMATHS_H

#ifdef __cplusplus
extern "C" {
#endif

// Main timezone functions
unsigned char apply_timezone(int* time, int* date, int tz);
void add_timezone_offset(int* time, int* date, int offset);

// Helper functions which may come in handy
void add_sub_days(unsigned char* day, unsigned char* month, unsigned char* year, int num_days);

#ifdef __cplusplus
}
#endif

#endif	/* TZMATHS_H */

