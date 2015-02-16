<?php

  function calc_intermediate_point_dist(&$a, &$b, &$c) {
    if( ($a[0] == $b[0] && $a[1] == $b[1]) || ($b[0] == $c[0] && $b[1] == $c[1]) )
      return 0;
    $dist_ac_b = ($c[1] - $b[1]) * $a[0] - ($c[0] - $b[0]) * $a[1] + $c[0]*$b[1] - $c[1]*$b[0];
    if( $dist_ac_b < 0 )
      $dist_ac_b = -$dist_ac_b;
    $dist_ac_b /= sqrt(($c[1] - $b[1]) * ($c[1] - $b[1]) + ($c[0] - $b[0]) * ($c[0] - $b[0]));
    return $dist_ac_b;
  }
  function calc_dist(&$a, &$c) {
    return sqrt(($c[1] - $a[1]) * ($c[1] - $a[1]) + ($c[0] - $a[0]) * ($c[0] - $a[0]));
  }

  function get_poly_circum(&$coords) {
    $n = count($coords);
    $ret = 0;
    for( $i = 0; $i < $n-1; ++$i ) {
      $ret += calc_dist($coords[$i], $coords[$i+1]);
    }
    return $ret + calc_dist($coords[$n-1], $coords[0]);
  }

  $GLOBALS["index"] = 0;
  function reduce_coords(&$c) {
    $ret = array();
    $circum = get_poly_circum($c);
    $area_est = ($circum / 6.283) * ($circum / 6.283) * 6.283 / 2;
    $i = 0;
    $n = count($c);
    while( $i < $n - 2 ) {
      $j = $i;
      while( $j < $n - 2 ) {
        $dist_ac_b = calc_intermediate_point_dist($c[$i], $c[$j+1], $c[$j+2]);
        $dist_ac = calc_dist($c[$i], $c[$j+2]);
        if( $dist_ac_b < $circum / 65536 )//&& $dist_ac_b * $dist_ac / 2 < $area_est / 9000000 )
#        if( $dist_ac_b < $circum / 1000000 && $dist_ac_b * $dist_ac / 2 < $area_est / 100000000 )
          ++$j;
        else
          break;
      }
      array_push($ret, $c[$i]);
      $i = $j + 1;
    }
    array_push($ret, $c[$i]);
    if( $i < $n-1 )
      array_push($ret, $c[$i+1]);
    ++$GLOBALS["index"];
    return $ret;
  }
  function compute_dst_delta($a, $b) {
    $x = ($a%100) + (($a/100)>>0) * 60;
    $y = ($b%100) + (($b/100)>>0) * 60;
    return $x - $y;
  }

  /*
    Encoding scheme:
      * Units are ten thousandths of degrees, resolution is around 12m
      * If first two bits are not both one but the 16 bit value is <40960, value is (dx + 100) + (dx + 100) * 201 with -100 <= dx <= 100 & -100 <= dy <= 100
      * If first two bits are not both one but the 16 bit value is >=40960, subtract 40960, multiply by 256 and add the following byte. Value is (dx + 723) + (dy + 723) * 1447 with -723 <= dx <= 723 & -723 <= dy <= 723
      * If first two bits are one and third is zero, 29 bits follow, value is (dx + 10000) + (dy + 10000) * 20001 with -10000 <= dx <= 10000 & -10000 <= dy <= 10000
      * If the first three bits are one and the fourth is zero, 44 bits follow, 22 bits each of lat & long 0 .. 3599999
      * If the first four bits are one and the fifth is zero, the current polygon ends and another starts in the same timezone (unless it's the end of the data)
      * Otherwise, the current polygon (if any) ends. Skip that byte and read the next for the new time zone ID. Its polygons follow.
  */

  function compress_coords(&$coords) {
    $ret = "";
    $n = count($coords);
    $last = array(0, 0);
    for( $i = 0; $i < $n; ++$i ) {
      $dx = $coords[$i][0] - $last[0];
      $dy = $coords[$i][1] - $last[1];
      if( abs($dx) <= 0.01 && abs($dy) <= 0.01 ) {
        $val = round($dx * 10000 + 100) + round($dy * 10000 + 100) * 201;
        $ret .= pack("v", $val*4/3);
        if( (($val*4/3)&3) == 3 )
          throw new Exception("aaaa");
      } else if( abs($dx) <= 0.0723 && abs($dy) <= 0.0723 ) {
        $val = round($dx * 10000 + 723) + round($dy * 10000 + 723) * 1447;
        $ret .= pack("vC", (($val>>8)+40960)*4/3, $val&255);
        if( ((($val>>8)*4/3+40960)&3) == 3 )
          throw new Exception("bbbb");
      } else if( abs($dx) <= 1 && abs($dy) <= 1 ) {
        $val = round($dx * 10000 + 10000) + round($dy * 10000 + 10000) * 20001;
        $ret .= pack("V", ($val<<3)|3);
      } else {
        if( $dx < -180 || $dx > 180 || $dy < -180 || $dy > 180 )
          throw new Exception("value out of range");
        $val1 = round($dx * 10000 + 1800000);
        $val2 = round($dy * 10000 + 1800000);
        $ret .= pack("vvv", ($val1<<4)|7, ($val1>>12)|($val2<<10), $val2>>6);
      }
      $last[0] += round($dx, 4);
      $last[1] += round($dy, 4);
    }
    return $ret;
  }

  $tzs = json_decode(file_get_contents(empty($argv[1]) ? "mytzs.json" : $argv[1]), true);
  $bytes = 0;
  $ret = "";
  $tzid = 0;
  $tznum = 0;
  $tz_info = array();
  $tz_map = array();
  $tz_names = array();
  $dst_types = array();
  $dst_deltas = array();
  $timezones = array();
  $tz_map_names = array();
  $tz_descs = array();
  $num_dst = 0;
  $num_dst_deltas = 0;
  foreach( $tzs as $tz => &$data ) {
    $first = TRUE;
    $timezone_desc = null;
    array_push($tz_names, $tz);
    foreach( $data as &$coords ) {
      if( $timezone_desc === null ) {
        $timezone_desc = $coords;
        array_push($tz_descs, $timezone_desc);
        if( empty($tz_info[$timezone_desc]) ) {
          $tz_info[$timezone_desc] = $tznum++;

          if( preg_match("/^([+-]\d{4})(?:,([+-]\d{4}),([a-zA-Z]+)DST)?\$/", $timezone_desc, $matches) ) {
            $normal_offset = $matches[1];
            $tzoff = preg_replace("/^(\s*[+-])0/", " \$1", preg_replace("/^(\s*[+-])0/", " \$1", preg_replace("/^(\s*[+-])0/", " \$1", $normal_offset)));
            if( count($matches) > 2 ) {
              $dst_delta = compute_dst_delta($matches[2], $normal_offset);
              $dst_type = $matches[3];
              if( empty($dst_types[$dst_type]) )
                $dst_types[$dst_type] = $num_dst++;
              if( empty($dst_deltas[$dst_delta]) )
                $dst_deltas[$dst_delta] = $num_dst_deltas++;
              array_push($timezones, $tzoff."^DST_".strtoupper($dst_type)."^DST_DELTA_".$dst_delta);
            } else {
              array_push($timezones, $tzoff);
            }
          } else {
            error_log("Timezone description doesn't match expected format: $timezone_desc");
            exit(-1);
          }
        }
        $tz_map[$tzid] = $tz_info[$timezone_desc];
        if( empty($tz_map_names[$tz_info[$timezone_desc]]) )
          $tz_map_names[$tz_info[$timezone_desc]] = $tz;
        else
          $tz_map_names[$tz_info[$timezone_desc]] .= ", ".$tz;
        continue;
      }
      if( $first && $tzid != 0 ) {
        $ret .= pack("v", 255|($tzid<<8));
      } else if( !$first ) {
        $ret .= pack("C", 15);
      }
      $ret .= compress_coords(reduce_coords($coords));
      $first = FALSE;
    }
    ++$tzid;
  }
  $n = strlen($ret);
  error_log("Compressed size: $n bytes vertices + ".(count($tz_map)+count($timezones)*4)." bytes timezone data = ".sprintf("%.1f", ($n+count($tz_map)+count($timezones)*4)/1024.0)." kB total\n");
  $longest_off = 0;
  foreach( $dst_types as $type => $index ) {
    if( strlen($type) > $longest_off )
      $longest_off = strlen($type);
  }
  $longest_tz = 0;
  foreach( $timezones as $index => $type ) {
    if( strlen($type) > $longest_tz )
      $longest_tz = strlen($type);
  }
  if( !empty($argv[2]) && $argv[2] == "decimate" ) {
    print("{\n");
    $i = 0;

    $longest_tz_name = 0;
    foreach( $tzs as $tz => &$data ) {
      if( strlen($tz) > $longest_tz_name )
        $longest_tz_name = strlen($tz);
    }
    $longest_tz_desc = 0;
    foreach( $tz_descs as $desc ) {
      if( strlen($desc) > $longest_tz_desc )
        $longest_tz_desc = strlen($desc);
    }
    foreach( $tzs as $tz => &$data ) {
      print('"'.preg_replace('/_/', '/', $tz).'"'.str_repeat(" ", $longest_tz_name - strlen($tz)).':["'.$tz_descs[$i].'",'.str_repeat(" ", $longest_tz_desc - strlen($tz_descs[$i]))."[");//':["'."hmm".'",'.str_repeat(" ", $longest_tz - strlen("hmm")).implode(',', $coords).']'.($i == $n ? '' : ',')."\n");
      $first = TRUE;
      $comma = "";
      foreach( $data as &$coords ) {
        if( $first ) {
          $first = FALSE;
          continue;
        }
        $c = reduce_coords($coords);
        error_log("$tz ".count($coords)." => ".count($c));
        print($comma."[");
        $subcomma = "";
        foreach( $c as $c2 ) {
          print($subcomma.$c2[0].",".$c2[1]);
          $subcomma = "],[";
        }
        print("]");
        $comma = "],[";
      }
      ++$i;
      print("]]".($i == count($tzs) ? "" : ",")."\n");
    }
    print("}\n");
  } else {
    print("\n");
    foreach( $dst_types as $type => $index ) {
      printf("#define DST_".strtoupper($type).str_repeat(" ", $longest_off + 3 - strlen($type))."0x%08x\n", 0x40000000>>($index-1));
    }
    foreach( $dst_deltas as $delta => $index ) {
      printf("#define DST_DELTA_".$delta.str_repeat(" ", 4 - strlen($delta))."0x%08x\n", $index == 1 ? 0 : (0x40000000>>(count($dst_types)+$index-2)) );
    }
    print("\n");
    print("#ifdef __cplusplus\n");
    print("extern \"C\" {\n");
    print("#endif\n");
    print("\n");
    print("#ifdef INCLUDE_TZ_DATA\n");
    print("const unsigned char tz_map[".count($tz_map)."] = { ".implode(", ", $tz_map)." };\n\n");
    print("const unsigned long timezones[".count($timezones)."] = {\n");
    $i = 0;
    foreach( $timezones as $timezone ) {
      print("\t\t".$timezone.($i == count($timezones)-1 ? " " : ",").str_repeat(" ", $longest_tz + 1 - strlen($timezone))."// ".$tz_map_names[$i]."\n");
      ++$i;
    }
    print("};\n");
    print("#else//INCLUDE_TZ_DATA\n");
    print("extern const unsigned char tz_map[".count($tz_map)."];\n\n");
    print("extern const unsigned long timezones[".count($timezones)."];\n");
    print("#endif//INCLUDE_TZ_DATA\n");
    print("\n");
    print("#ifdef INCLUDE_TZ_NAMES\n");
    print("const char* tz_names[".count($tz_names)."] = {\n");
    $i = 0;
    foreach( $tz_names as $name ) {
      print("\t\t\"".$name."\"".(++$i == count($tz_names) ? "" : ",")."\n");
    }
    print("};\n");
    print("#endif//INCLUDE_TZ_NAMES\n\n");
    print("\n");
    print("#ifdef INCLUDE_TZ_COORDS\n");
    print("const unsigned char tz_coords[$n] = {");
    for( $i = 0; $i < $n; $i += 16 ) {
      $comma = "\n\t";
      for( $j = 0; $i+$j < $n && $j < 16; ++$j ) {
        $val = unpack("C", substr($ret, $i+$j, 1));
        printf("%s%d", $comma, $val[1]);
        $comma = ", ";
      }
      if( $j == 16 && $i+$j < $n )
        print(",");
    }
    print("};\n");
    print("#else//INCLUDE_TZ_COORDS\n");
    print("extern const unsigned char tz_coords[$n];\n");
    print("#endif//INCLUDE_TZ_COORDS\n");
    print("\n");
    print("#ifdef __cplusplus\n");
    print("}\n");
    print("#endif\n");
  }
?>
