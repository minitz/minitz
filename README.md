
# minitz v1.0 - MINiaturised International TimeZone database

Distributed under the Apache License 2.0 (see LICENSE file).


1. Table of Contents

* Section  1 - this section
* Section  2 - What is minitz?
* Section  3 - Why minitz was developed
* Section  4 - How minitz works
* Section  5 - How to use minitz
* Section  6 - How to modify minitz
* Section  7 - Limitations of minitz
* Section  8 - Possible future upgrades for minitz
* Section  9 - What to do if you want to include minitz in your own project
* Section 10 - How you can help with minitz
* Section 11 - Indicental downloads
* Section 12 - How to contact the author(s) of minitz

2. What is minitz?

  Minitz is a free, open-source database and accompanying C software library
  which you can use to determine time zone data from a latitude/longitude
  pair. Given this time zone data, it can then convert a UTC time/date into
  the local time and date.

  Minitz requires approximately 200KB of storage and <1KB of RAM. Its
  performance is good enough that it will generally come up with the correct
  time zone after executing somewhere in the range of 100-1000 million
  instructions. The code is arranged to perform the time zone search in
  manageably small chunks of processing time, suitable for use in a
  real-time or co-operative multi-tasking system.

  It also includes some helper functions to extract the latitude, longitude,
  UTC time and UTC date from the NMEA serial stream from a GPS module.

  Essentially, this means that with a GPS module and minitz, you can quickly
  and easily determine the local time just about anywhere on Earth with zero
  user input. A sub-$4 microcontroller such as a PIC32 or ARM Cortex with at
  least 256KB flash is capable of running minitz and should give a result
  within a few seconds of getting a valid GPS fix.

  Minitz it can also be used on PCs, laptops, mobile phones or any other
  sufficiently powerful computing device. Its low overhead means it won't
  bloat your software.

  Even if you aren't interested in calculating the time zone from GPS data,
  you may still find the minitz source code useful as it also contains
  routines to apply the sometimes complex Daylight Savings rules for various
  countries.

3. Why minitz was developed

  Free time zone databases already exist however they have significant
  limitations. Probably the best well-known is tz_world. This is a very
  comprehensive database but it is huge - many megabytes and will never
  fit in the flash memory of a cheap microcontroller (at least not until
  micros become a lot more powerful). The large amount of data also greatly
  increases the computational overhead.

  The reason tz_world is so large is that it stores every region of every
  country separately, even if they are in the same time zone, so there's a
  lot of unnecessary data for the task at hand. Also, it includes the
  coastal details and all islands, when we really don't need that
  information to determine the time zone on or near land. All we need to
  know is the border of each zone with distinct time zone rules. Sea borders
  are not critical, as long as they will give reasonable results (ie,
  lat/long pairs at sea will generally return a nearby land time zone).

  tz_world also stores land border data twice, once for each adjoining
  region. Minitz avoids that. tz_world also includes many redundant points,
  eg, long straight borders may be stored as 50 or 100 separate lat/long
  pairs. This is simply wasteful.

  tz_world contains a compressed database of just enough data to accurately
  determine the time zone (and thus local time) from a land-based lat/long
  and nothing more. Its database is thus under 200KB and relatively quick to
  search. Minitz also provides working example code suitable for compiling
  on a 32-bit microcontroller or PC. Thus it's much less work to get linked
  into your project.

  Minitz was originally developed for use in the Silicon Chip Nixie Clock
  Mk2 which includes the option of fitting a GPS module for accurate and
  automatic timekeeping. For more information on this project, see the
  following URLs: [Part 1](http://www.siliconchip.com.au/Issue/2015/February/6-Digit+Retro+Nixie+Clock+Mk.2%2C+Pt.1)  [Part 2](http://www.siliconchip.com.au/Issue/2015/March/6-Digit+Retro+Nixie+Clock+Mk.2%2C+Pt.2)

  Most of the C code was written from scratch although the line intersection
  routine is based on the one from this web page: [StackOverflow](http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect)

  Thanks to use "iMalc" for the code.

  Most of the database was drawn by hand although a few sections are based
  on data tz_world.

4. How minitz works

  The main database consists of a number of polygons made up of lat/long
  co-ordinate pairs. This database is distributed in two forms, as a JSON
  file which is human-readable and easily mungeable (plus some PHP scripts
  to convert it to/from KML format) and a delta-compressed binary format in
  a C header file which is just under 200KB. The latter is generated from
  the former, again using a small PHP script. You only need to run the PHP
  scripts   if you plan to visually examine or modify the time zone data.

  The C code contains a routine to progressively search these polygons and
  determine which a given lat/long resides within. Assuming the given
  lat/long is found to be within a polygon, this then gives an index to look
  up the associated time zone in a table. Some time zones are nothing more
  than a UTC offset, while others include two UTC offsets plus some flags
  which allows further code to determine whether daylight savings is in
  operation in that zone and select the appropriate offset.

  Thus you feed in a lat/long and get back a zone index. You can then feed
  in the UTC time and/or date along with the zone index to get local time.
  The latter step is quite fast and you only need to do another zone look-up
  if the lat/long co-ordinates change significantly (or you can do this
  periodically, in the background).

  The polygon search works quite simply. Each polygon is checked in turn.
  First, the minimum and maximum lat/long bounding box is computed. If the
  search lat/long is not within this, the polygon check is skipped, saving
  calculation time.

  Assuming the search lat/long is within this bounding box, it may or may
  not be within this polygon. A further search is made by first picking a
  random lat/long pair outside the polygon's bounds and an imaginary ray is
  cast from the search lat/long to this randomly selected one. The number of
  intersections between this ray and each edge of the polygon is then
  computed. If the number of intersections is odd, the search lat/long is
  within the polygon and so the time zone index is returned. Otherwise, the
  search continues until all polygons have been checked. If that happens,
  the search lat/long is not within any known time zone and is probably
  either at sea or on Antarctica. In this case, if there is a nearby time
  zone co-ordinate (within +/-1 degree or so), the nearest time zone is
  returned. Otherwise a "no match" result is returned.

  Note that to keep the database small, we take heavy advantage of the
  "first-out" nature of the search. Each zone polygon can overlap earlier
  polygons harmlessly, since these regions will have already been excluded
  before they are searched. Thus, when there is a shared border, the first
  polygon must define it accurately while the second one can be sloppy in
  order to minimise the number of points stored, saving space in the
  database.

5. How to use minitz

  The file "timezones.c" and accompanying header file "timezones.h" contain
  the code for determining a time zone from a lat/long while the time zone
  offset and daylight savings calculation code is in "tzmaths.c" and
  "tzmaths.h". The supplied makefile builds an executable with this test
  code.  This can be run to perform a regression test.  It goes through a
  couple of hundred city lat/longs and checks that the correct time zone is
  found for each.  It also prints the computed local time for each city,
  based on the PC's UTC time.

  All you need to do is look at the very simple "get_timezone" function
  which takes a lat/long and performs the search. This is a 'blocking'
  function; in an asynchronous environment (eg, co-operative multi-tasking)
  it's simply a matter of calling continue_get_timezone with the maximum
  number of edge comparisons you wish it to perform as the parameter. (You
  must initialise first; see the code). If the return value is -1, the
  search is ongoing. If the return value is -2 then the search is complete
  and no match is found. Any other return value means that the time zone has
  been found and this value is its index. You can then pass this value to
  apply_timezone() in order to convert the UTC time/date (in BCD-ish
  format; again, see test code) to local time/date.

  The lat/long format used is a fixed-point signed decimal format with a
  resolution of 1/10000 of a degree. So 57N 37W is represented as 570000,
  -370000.

  To expand on the time/date format, the time value is simply `seconds +
  minutes * 100 + hours * 10000`. Date is similarly `(year-2000) + month[1-12] *
  100 + day[1-31] * 10000`. Date must be provided in order to correctly
  compute daylight savings.

6. How to modify minitz

  The most likely modification you may want to make is to change the time
  zone database. This can be done easily using Google Earth, as long as you
  have a PHP commandline interpreter installed. Assuming so, execute:

  `php extractkml.php`

  This will read the contents of mytsz.json (the main database) and generate
  a file called "Timezones.kml". It's then simply a matter of opening the
  kml file in Google Earth. Each polygon will appears separately under the
  KML file, with its time zone data in the description. You can view the
  vertices and edit them by right-clicking a polygon and choosing
  "Properties".

  Note that as stated in section 4, overlap regions tend to be "sloppy" to
  save space. Earlier polygons (ie, those higher up the list) have priority.
  Note also that if you don't have a PHP interpreter, you can download the
  supplied Timezones.kml database and open it directly, however you will
  still need PHP to convert it back into C data should you wish to do so
  later.

  If you need to add any polygons, make sure they are in the right order as
  per the explanation above and also make sure the description is set
  correctly for the time zone.

  Once you've finished making changes, save the file, then run:

  `php importkml.php`

  This will then overwrite mytzs.json with the new data from Google Earth.
  To generate the new C database (tz_coords.h), run:

  `php compresstz.php > tz_coords.h`

  This will be performed automatically if the JSON file has been updated
  whenever "make" or "make test" are executed, prior to compiling the C
  code. Once the code has been re-compiled, it will use the new data. You
  should run the regression test ("make test") after making changes but note
  that if you added any polygons or changed the order, you will also need to
  re-arrange the regression test order to suit.

7. Limitations of minitz

  Firstly, the 1/10000 degree resolution limits accuracy to around 11m. This
  isn't likely to be an issue, few people live that close to a timezone
  border and the source data probably isn't that accurate anyway.

  The two main limitations on accuracy are source data accuracy and the
  number of points used for the time zone boundary polygons. There's also
  the issue that time zones frequently change over time and some time zone
  rules are not easily computed for future dates.

  Also note that there are some lat/long pairs which will return no time
  zone data.

  a) Data accuracy: For some reason, when you plot the data from say
  tz_world on Google Earth, it doesn't line up with the borders drawn on
  Google Earth.  Sometimes it's quite far out - hundreds of metres or more. 
  What's even worse is that often, the borders follow a river and yet you
  will find that NEITHER the tz_world nor Google Earth borders match up with
  the apparent course of the river! Rivers do move but not usually when they
  are shored up (eg, where they pass through a town or city) so this
  probably represents an inaccuracy somewhere but I don't know where it is
  for sure. I've done my best to guess the correct location for the borders
  but I would estimate the worst-case error is around 1km.

  If you can help in checking for border accuracy or improving accuracy of
  the database, please do so (see Section 9).

  b) Number of points: in many regions I've made the borders as accurate as
  the source data allows, especially in countries with large populations or
  a high level of development. However less points are used in less
  populated areas to save database space as it's unlikely to make any
  difference in real world use. In countries like Africa, the number of
  vertices per kilometre is lower than, say, western Europe.

  Again, anyone willing to help improve the distribution of vertices is
  welcome. See Section 9 below. But practically speaking, I believe the
  accuracy of the present database is quite sufficient.

  c) Time zone changes: I've tried to ensure that all data is accurate as
  per February 2015 however it's possible that some of the source data I've
  used is out of date. Changes to timezones can occur at any time. I will
  attempt to keep minitz up to date but would appreciate help.

  Please check the database to ensure that your own local time zone is
  correct, and any others you are familiar with. Please also add extra
  co-ordinates to the regression test. See Section 9 below if you wish to
  help and contact me (Section 12) if you find any problems.

  d) Difficult time zone rules: unfortunately, some locations such as
  various Middle Eastern countries and parts of Brazil have daylight savings
  rules that depend on religious holidays (eg, Ramadan and Easter) which are
  not only difficult to compute but not necessarily known in advance and may
  change without notice. A reasonable attempt has been made to calculate
  these dates however it's possible they will be off in some years. There
  isn't much we can do about this. In my opinion, daylight savings start/end
  times should be calculable and deterministic but I'm not in charge of the
  rules.

  e) Lat/long pairs not in any time zone: I believe all Earth's land masses
  are included in the database, except for Antarctica. Some small islands
  may have been accidentally left off. If so, please let me know and I will
  add them.

  Ideally, all sea locations should be handled as there are defined time
  zones at sea however this has not been a priority. Locations at sea will
  probably be covered in version 1.1 - see Section 8.

  I'm not sure what to do about Antarctica. Not many people live there and
  while theoretically there are time zones, it's pretty arbitrary and
  meaningless. If you have any suggestions, feel free to write in.

8. Possible future upgrades for minitz

  a) As stated above, the next version will hopefully have full ocean
  coverage. This will leave Antarctica as the only region not covered,
  unless I think of a clever way to deal with it.

  b) I'm not sure that the time zones of all Pacific islands are correct.
  I'm hoping to check this and many any necessary corrections for the next
  release.

  c) It's possible to improve accuracy of various time zones by either
  moving the vertices closer to the correct locations or adding more
  vertices. There will probably be no end to this, I won't spend a lot of
  time making such improvements but if volunteers wish to do so, by all
  means go ahead and please send me the revised polygon data.

  d) Similarly it should be possible to shrink the data somewhat without
  sacrificing any accuracy. And as for (c) it's probably an endless task
  with diminishing returns but I would be very happy for volunteers to do
  this and send me revised data. Note that the compresstz.php script does
  remove obviously redundant vertices (ie, collinear or nearly-collinear
  points) and this is tunable, so the database could probably be shrunk
  10-20% with no real loss in accuracy by simply tuning this parameter. But
  manually tweaking the vertices is bound to help also.

  e) I am hoping to offer a PIC32 example program in the near future which
  will acquire data from a serial GPS unit and dump the local time to a
  serial console. This should make it very easy for hardware designers to
  get up and running.

9. What to do if you want to include minitz in your own project

  Simply compile in timezones.c (which requires tz_coords.h) and then use
  something similar to the test code at the bottom of the C file. Note that
  if you have storage space and wish to be able to display the detected
  timezone, you can #define INCLUDE_TZ_NAMES and this will give you a char**
  array called tz_names[] which you can look up the time zone index in to
  give a name. But this name won't always make a lot of sense as the zones
  tend to cover multiple regions/countries.

  Otherwise, tz_names is not included to save space.

  If you want to use minitz in a non-C project, you will either need to
  compile the C code into a library and link against it (many languages
  support this, see the gcc docs [or your preferred C compiler] for how to
  generate a library file). Alternatively, you can translate the minitz code
  into your preferred language and munge the tz_coords.h data to suit. If
  you do this, feel free to send send me the (tested and working!) results
  and I'll include it in the package. See Section 12.

  You are free to include minitz in your project, whether it's a commercial
  product or not, with no strings attached. But I would appreciate it if you
  dropped me a line and let me know you are doing so, to satisfy my
  curiousity. See section 12.

10. How you can help with minitz

  Basically, the main things I would like help with are:

  a) Checking that the time zone polygons, offsets and daylight savings
  rules are correct. Please check the region where you live; add its
  lat/long to the regression test in the appropriate region, compile it, run
  it and check that the local time/date displayed is correct. If not, please
  let me know. If you can correct any problems in the database, please do so
  and send me the diff. If you have the time and desire, please check other
  regions too - open the data in Google Earth as described above (Section 6)
  and check away.

  b) Checking that the time zone polygons are as accurate as possible and do
  not contain redundant vertices. Please note that this does not necessarily
  mean I want you to add a sqillion new vertices to a region to make it
  super-duper accurate as this may bloat the database! Use your judgement.
  More vertices are called for in more heavily populated regions. If you can
  add a handful of vertices and tweak some others to improve accuracy
  without growing the database much, that's probably worthwhile and please
  send me the results.

  c) Shrinking the database. If you find a good way to do this without
  affecting accuracy (much) please do let me know.

  d) Merging regions and adding sea coverage. I would very much appreciate
  help in expanding time zones to cover the appropriate sections of ocean. I
  will pobably do this myself if nobody else does but help would be good.

  See Section 12 below for details on how to send changes in.

11. Incidental downloads

  As a result of developing minitz, a few potentially useful kml polygons
  have been generated and these are available as separate downloads.  They
  are:

* China (PROC)/Taiwan (ROC) border data
* Bangladesh border data
* Paraguay border data
* Uruguay border data

  The data is available in the `Timezones.kml` file.

  Other useful boundaries could no doubt be extracted from the database but
  are not readily available in the present form.

12. How to contact the author(s) of minitz

  Please let me know if you have any questions or concerns, find any
  problems with minitz or are willing to help improve it. E-mail me at: minitz@users.noreply.github.com

  If you make changes to one or more polygons, ideally I would like you to
  send only those polygons you've changed or at least indicate which ones
  have been modified. If you change the C code, diffs would be good. If you
  make changes, it's best to send them in promptly in case someone else
  sends other changes which might conflict.

  I will release a new version of minitz once there are enough improvements
  or additions to justify it.

  Thanks and I hope you find minitz useful!

Nicholas Vinen.

