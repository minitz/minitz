test_prog: timezones.c timezones.h tzmaths.c tzmaths.h tz_coords.h
	g++ -Wall -Os -o test_prog -D TEST_MODE timezones.c tzmaths.c

tz_coords.h: mytzs.json compresstz.php
	php compresstz.php > tz_coords.h

test: test_prog
	./test_prog
