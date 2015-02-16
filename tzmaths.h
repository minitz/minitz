
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

