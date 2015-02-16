<?php

/* minitz - MINiaturised International TimeZone database v1.0
   
   Written by Nicholas Vinen. Please see README and LICENSE files.
   
   PHP code to convert JSON file to KML format. */

  $tzs = json_decode(file_get_contents(empty($argv[1]) ? "mytzs.json" : $argv[1]), true);
  $colours = array("000000", "7f0000", "007f00", "00007f", "7f7f00", "007f7f", "7f007f", "7f7f7f");

  $str = 
'<?xml version="1.0" encoding="UTF-8"?>'."\n".
'<kml xmlns="http://www.opengis.net/kml/2.2" xmlns:gx="http://www.google.com/kml/ext/2.2" xmlns:kml="http://www.opengis.net/kml/2.2" xmlns:atom="http://www.w3.org/2005/Atom">'."\n".
'<Document>'."\n".
'	<name>Timezones.kml</name>'."\n";
  $style_index = 1;
  $colour_index = 0;
  foreach( $tzs as $tz => &$data ) {
    $str .=
'	<Style id="mypoly'.($style_index++).'">'."\n".
'		<PolyStyle>'."\n".
'			<color>7f'.$colours[$colour_index].'</color>'."\n".
'			<colorMode>random</colorMode>'."\n".
'		</PolyStyle>'."\n".
'	</Style>'."\n";
    if( ++$colour_index == count($colours) )
      $colour_index = 0;
  }
  $str .=
'	<Folder>'."\n".
'		<name>Timezones</name>'."\n".
'		<open>1</open>'."\n";

  $style_index = 1;
  foreach( $tzs as $tz => &$data ) {
    $name = preg_replace("#/#", "_", $tz);

    $index = 1;
    $offset = null;
    foreach( $data as &$coords ) {
      if( $offset === null ) {
        $offset = $coords;
        continue;
      }
      $str .= 
'		<Placemark>'."\n".
'			<name>'.$name.'_'.($index++).'</name>'."\n".
'               <description>'.htmlspecialchars($offset).'</description>'."\n".
'			<styleUrl>#mypoly'.$style_index.'</styleUrl>'."\n".
'			<Polygon>'."\n".
'				<tessellate>1</tessellate>'."\n".
'				<outerBoundaryIs>'."\n".
'					<LinearRing>'."\n".
'						<coordinates>'."\n";
      foreach( $coords as &$coord )
        $str .= $coord[0].','.$coord[1].',0 ';
      $str .= "</coordinates>\n".
'					</LinearRing>'."\n".
'				</outerBoundaryIs>'."\n".
'			</Polygon>'."\n".
'		</Placemark>'."\n";
    }
    ++$style_index;
  }
    $str .=
'	</Folder>'."\n";
  $str .= '</Document>'."\n".
'</kml>'."\n";
  file_put_contents('Timezones.kml', $str);

?>
