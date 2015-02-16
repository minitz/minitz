<?php

/* minitz - MINiaturised International TimeZone database v1.0
   
   Written by Nicholas Vinen. Please see README and LICENSE files.
   
   PHP code to convert KML file to JSON format. */

  $GLOBALS['in_placemark'] = false;
  $GLOBALS['in_placemark_name'] = false;
  $GLOBALS['in_placemark_description'] = false;
  $GLOBALS['in_coordinates'] = false;
  $GLOBALS['placemark_name'] = null;
  $GLOBALS['placemark_description'] = null;
  $GLOBALS['coords'] = array();
  $GLOBALS['timezones'] = array();
  $GLOBALS['max_timezone_len'] = 0;

  function startElement($parser, $name, $attrs) {
    if( $name == 'PLACEMARK' ) {
      $GLOBALS['in_placemark'] = true;
      $GLOBALS['placemark_name'] = null;
      $GLOBALS['placemark_description'] = null;
    } else if( $GLOBALS['in_placemark'] && $name == 'NAME' ) {
      $GLOBALS['in_placemark_name'] = true;
    } else if( $GLOBALS['in_placemark'] && $name == 'DESCRIPTION' ) {
      $GLOBALS['in_placemark_description'] = true;
    } else if( $GLOBALS['in_placemark'] && $name == 'COORDINATES' ) {
      $GLOBALS['in_coordinates'] = true;
    }
  }

  function endElement($parser, $name) {
    if( $name == 'PLACEMARK' ) {
      $GLOBALS['in_placemark'] = false;
      $GLOBALS['placemark_name'] = null;
    } else if( $GLOBALS['in_placemark'] && $name == 'NAME' ) {
      $GLOBALS['in_placemark_name'] = false;
    } else if( $GLOBALS['in_placemark'] && $name == 'DESCRIPTION' ) {
      $GLOBALS['in_placemark_description'] = false;
    } else if( $GLOBALS['in_placemark'] && $name == 'COORDINATES' ) {
      $GLOBALS['in_coordinates'] = false;
    }
  }
 
  function contents($parser, $data) {
    if( $GLOBALS['in_placemark_name'] ) {
      $GLOBALS['placemark_name'] = $data;
    } else if( $GLOBALS['in_placemark_description'] ) {
      $GLOBALS['placemark_description'] = $data;
    } else if( $GLOBALS['in_coordinates'] && $GLOBALS['placemark_name'] !== null && $GLOBALS['placemark_description'] !== null ) {
      if( preg_match("/^(.*)_(\d+)\$/", $GLOBALS['placemark_name'], $matches) ) {
        if( empty($GLOBALS['coords'][$matches[1]]) )
          $GLOBALS['coords'][$matches[1]] = array();
        array_push($GLOBALS['coords'][$matches[1]], '[['.preg_replace("/\n/", '', preg_replace("/(?:,0|\],\[)\$/", ']]', preg_replace("/\s+/", "", preg_replace('/,0 /', '],[', $data)))));
        $GLOBALS['timezones'][$matches[1]] = $GLOBALS['placemark_description'];
        if( strlen($GLOBALS['placemark_description']) > $GLOBALS['max_timezone_len'] )
          $GLOBALS['max_timezone_len'] = strlen($GLOBALS['placemark_description']);
      }
    }
  }

  $parser = xml_parser_create('');
  xml_set_element_handler($parser, 'startElement', 'endElement');
  xml_set_character_data_handler($parser, 'contents');
  if( xml_parse($parser, file_get_contents(empty($argv[1]) ? 'Timezones.kml' : $argv[1])) ) {
    print("{\n");
    $i = 0;
    $c = $GLOBALS['coords'];
    $n = count($GLOBALS['coords']);

    $max_name_len = 0;
    foreach( $c as $name => &$coords ) {
      if( strlen($name) > $max_name_len )
        $max_name_len = strlen($name);
    }
    foreach( $c as $name => &$coords ) {
      ++$i;
      print('"'.preg_replace('/_/', '/', $name).'"'.str_repeat(" ", $max_name_len - strlen($name)).':["'.$GLOBALS['timezones'][$name].'",'.str_repeat(" ", $GLOBALS['max_timezone_len'] - strlen($GLOBALS['timezones'][$name])).implode(',', $coords).']'.($i == $n ? '' : ',')."\n");
    }
    print("}\n");
  } else {
    die(sprintf('XML error: %s at line %d', xml_error_string(xml_get_error_code($parser)), xml_get_current_line_number($parser)));
  }
  xml_parser_free($parser);
  
?>
