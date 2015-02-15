test_prog: timezones.c timezones.h tz_coords.h
	g++ -o test_prog -D TEST_MODE timezones.c

tz_coords.h: mytzs.json compresstz.php
	php compresstz.php > tz_coords.h

test: test_prog
	./test_prog
