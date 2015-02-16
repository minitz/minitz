
/* minitz - MINiaturised International TimeZone database v1.0
   
   Written by Nicholas Vinen. Please see README and LICENSE files.
   
   Main header. */

#ifndef TIMEZONES_H
#define	TIMEZONES_H

#ifdef __cplusplus
extern "C" {
#endif

void init_get_timezone(int lat, int lon);
int continue_get_timezone(int max_calcs);

#ifdef __cplusplus
}
#endif

#endif	/* TIMEZONES_H */

