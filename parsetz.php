<?
  function calc_error(&$a, &$b, &$c) {
    $x0 = $a[0];
    $y0 = $a[1];
    $x1 = $b[0];
    $y1 = $b[1];
    $x2 = $c[0];
    $y2 = $c[1];
    $dist_ac_b = ($y2 - $y1) * $x0 - ($x2 - $x1) * $y0 + $x2*$y1 - $y2*$x1;
    if( $dist_ac_b < 0 )
      $dist_ac_b = -$dist_ac_b;
    $dist_ac_b /= sqrt(($y2 - $y1) * ($y2 - $y1) + ($x2 - $x1) * ($x2 - $x1));
    $dist_ac = sqrt(($y2 - $y0) * ($y2 - $y0) + ($x2 - $x0) * ($x2 - $x0));
    return $dist_ac_b * $dist_ac / 2;
  }

  function reduce_coords(&$c) {
    $ret = array();
    $i = 0;
    while( $i < count($c) - 2 ) {
      $j = $i;
      while( $j < count($c) - 2 && calc_error($c[$i], $c[$j+1], $c[$j+2]) < 0.00002 )
        ++$j;
      array_push($ret, $c[$i]);
      $i = $j + 1;
    }
    array_push($ret, $c[$i]);
#    array_push($ret, $c[$i+1]);
    return $ret;
  }

  print("read\n");
  $raw = json_decode(file_get_contents("Timezone Files/tz_world.json"), true);
  print("process\n");
  $tzs = array();
  $polys = 0;
  $coords = 0;
  $rcoords = 0;
  foreach( $raw["features"] as $feature ) {
    $tz = $feature["properties"]["TZID"];
#    if( preg_match("#^(?:Australia|Pacific/(?:Auckland|Chatham))#", $tz) ) {
    if( preg_match("#^Europe/London#", $tz) ) {
      if( empty($tzs[$tz]) )
        $tzs[$tz] = array();
#      print("$polys ".count($feature["geometry"]["coordinates"][0])." =>\n");
#      flush();
#      $c = reduce_coords($feature["geometry"]["coordinates"][0]);
#      print(count($c).".\n");
#      flush();
      array_push($tzs[$tz], $feature["geometry"]["coordinates"][0]);
      ++$polys;
      $coords += count($feature["geometry"]["coordinates"][0]);
#      $rcoords += count($c);
    }
  }
  $raw = null;
  print(json_encode($tzs));
/*
  print(count($tzs)." timezones, $polys polygons, $coords coordinates\n");
  foreach( $tzs as $tz => &$data ) {
    foreach( $data as &$coords ) {
      $before = count($coords);
      $coords = reduce_coords($coords);
      $after = count($coords);
      print("$tz: before $before, after $after.\n");
      for( $i = 1; $i < count($coords); ++$i ) {
        $dx = $coords[$i][0] - $coords[$i-1][0];
        $dy = $coords[$i][1] - $coords[$i-1][1];
        print("  $dx $dy\n");
      }
    }
  }
*/
?>
