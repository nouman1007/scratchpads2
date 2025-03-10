<?php

/**
 * Theme function for generating a basic map.
 * @param $variables - see the arguments for gm3_get_map
 */
function theme_gm3_map($variables){
  $map = gm3_get_map($variables);
  return drupal_render($map);
}

/**
 * If the points collection is flagged as a convex hull, convert the points
 * into a polygon.
 *
 * Todo: Investigate whether this might be better done with Javascript
 * @param $libraries The gm3 libraries argument
 */
function _gm3_convex_hull_to_polygon(&$libraries) {
  if(isset($libraries['point']['convexhull']) && $libraries['point']['convexhull']){
    // We need to add a convex hull polygon. Lets do so.
    if(gm3_load_geophp()){
      $points = array();
      foreach($libraries['point']['points'] as $point){
        $points[] = new Point($point['latitude'], $point['longitude']);
      }
      $multipoint = new MultiPoint($points);
      $convexhull = json_decode(@$multipoint->convexHull()->out('json'));
      $poly = array();
      foreach($convexhull->coordinates[0] as $latlng){
        $poly[] = array(
          $latlng[1], // WHY OH WHY is this the reverse order of what we expect?
          $latlng[0]
        );
      }
      $libraries['polygon']['polygons'][] = array(
        'polygon' => $poly,
        'editable' => FALSE,
        'content' => t('Convex hull for the points within it.')
      );
    }
  }
}

/**
 * Generate the markup for the tools section of the map (the buttons on the left)
 *
 * @param $id string The map ID
 * @param $libraries string[] The dictionary of libraries to be included in the map
 */
function _gm3_generate_tools($id, $libraries) {
  // Generate the HTML for each button
  $tools = array_map(
    function($tool) use ($id) {
      return theme(
        "gm3_${tool}_button",
        [ 'id' => $id ]
      );
    },
    array_keys($libraries)
  );

  // Generate the wrapper
  return '<div class="gm3-tools">' .
    '<ul id="toolbar-' . $id . '">' .
      '<li class="gm3-clicked">' .
        '<button type="button" data-gm3-class="default" class="gm3-tools-button">' .
          '<p>' . t('Move') . '</p>' .
        '</button>' .
      '</li>' .
      '<li>' .
        implode("</li><li>", $tools) .
      '</li>' .
    '</ul>' .
  '</div>';
}

/**
 * Function to return a renderable array for a map.
 * @param $variables = [
 *   'id' => the html ID for the map
 *   'libraries' => javascript libraries to include and their settings - see hook_gm3_libraries_alter
 *   'settings' => General javascript settings for map; see gm3_settings
 *   'tools' => Boolean - whether to render the edit tools for the given libraries
 * ]
 */
function gm3_get_map($variables){
  // Could we get this dynamically from hook_theme?
  $id = $variables['id'] ?? 'gm3-map';
  $settings = $variables['settings'] ?? [];
  $libraries = $variables['libraries'];
  $tools = $variables['tools'] ?? false;

  // Return the markup and the CSS files
  return array(
    // Keep some gm3 settings in the render array in case we want to modify them
    // before render
    '#gm3' => [
      'id' => $id,
      'libraries' => $libraries,
      'settings' => $settings,
      'tools' => $tools
    ],
    // Once rendered we want to load the javascript libraries
    // and build the toolbar
    '#post_render' => [
      '_gm3_map_post_render'
    ],
    '#attached' => array(
      'css' => array(
        drupal_get_path('module', 'gm3') . '/css/gm3.css'
      ),
      'library' => [
        ['gm3', 'gm3']
      ]
    ),
    '#markup' =>
        // NB: We add tools and another outer wrapper in _gm3_map_post_render
        '<div class="gm3-map-wrapper">' .
          '<div id="' . $id . '"></div>' .
        '</div>'
  );
}

/**
 * When it's time to render the map, include the necesary tool buttons and load the js
 */
function _gm3_map_post_render(&$children, &$element) {
  $gm3 = $element['#gm3'];
  $id = $gm3['id'];
  // Add the javascript libs and settings
  gm3_add_js($id, $gm3['settings'], $gm3['libraries']);

  $tools = $gm3['tools'] ? _gm3_generate_tools($id, $gm3['libraries']) : '';

  return '<div class="gm3-container">' . $tools . $children . '</div>';
}

/**
 * Polygon tools button
 */
function theme_gm3_polygon_button($variables){
  theme('gm3_beautytip', array(
    'selector' => '.gm3-tools-button[data-gm3-class="polygon"]',
    'text' => t('Click to enable adding/removing polygons.  Many sided polygons are simply added by clicking on the extreme points of the polygon.')
  ));
  return '<button type="button" data-gm3-class="polygon" class="gm3-tools-button"><p>+ ' . t('Polygon') . '</p></button>';
}

/**
 * Polyline tools button
 */
function theme_gm3_polyline_button($variables){
  theme('gm3_beautytip', array(
    'selector' => '.gm3-tools-button[data-gm3-class="polyline"]',
    'text' => t('Click to enable adding/removing lines.  Lines are added by clicking on the map once to start a line, and then on every point where there is an angle in the line.')
  ));
  return '<button type="button" data-gm3-class="polyline" class="gm3-tools-button"><p>+ ' . t('Line') . ' +</p></button>';
}

/**
 * Polyline tools button
 */
function theme_gm3_rectangle_button($variables){
  theme('gm3_beautytip', array(
    'selector' => '.gm3-tools-button[data-gm3-class="rectangle"]',
    'text' => t('Click to enable adding/removing rectangles.<br/>Add a rectangle by clicking on the map at the top left of a rectangle, and then at the bottom right.')
  ));
  return '<button type="button" data-gm3-class="rectangle" class="gm3-tools-button"><p>+ ' . t('Rectangle') . '</p></button>';
}

/**
 * Point tools button
 */
function theme_gm3_point_button($variables){
  theme('gm3_beautytip', array(
    'selector' => '.gm3-tools-button[data-gm3-class="point"]',
    'text' => t('Click to enable adding/removing points.<br/>Add points by left clicking the map.<br/>Remove a point by right clicking it.')
  ));
  return '<button type="button" data-gm3-class="point" class="gm3-tools-button"><p>+ ' . t('Point') . '</p></button>';
}

/**
 * Theme a GM3 beautytip.
 */
function theme_gm3_beautytip($variables){
  if(function_exists('beautytips_add_beautytips')){
    $options['bt_drupal_help_page'] = array(
      'cssSelect' => $variables['selector'],
      'text' => $variables['text'],
      'closeWhenOthersOpen' => TRUE,
      'shrinkToFit' => TRUE,
      'width' => '300px',
      'spikeLength' => '10',
      'overlap' => '3',
      'positions' => 'right',
      'fill' => '#eee',
      'cornerRadius' => '3',
      'shadow' => TRUE
    );
    beautytips_add_beautytips($options);
  }
}

/**
 * Hook to add extra layers to a map component
 */
function hook_gm3_libraries_alter(&$libraries) {
  // Assumes your library file is at exmaple_module_name/js/example_lib.js
  $libraries['example_lib'] = [
    'module' => 'example_module_name', // Todo: We should be able to autodetect this
    'other_settings' => [
      'custom_settings' => 'for your module'
    ]
  ];
}

/**
 * Adds the required libraries for the given map
 */
function gm3_add_js($id, $settings = [], $libraries = []){
  $settings = array_merge(gm3_settings(), $settings ?? []);

  // Make sure we can render convex hulls
  _gm3_convex_hull_to_polygon($libraries);

  // Allow other modules to add their own libraries
  // See hook_gm3_libraries_alter for an example
  drupal_alter('gm3_libraries', $libraries);

  // Iterate through and requrie the library files
  if(is_array($libraries)){
    foreach($libraries as $library => $lib_settings){
      // If not module specified, assume gm3
      $module = $lib_settings['module'] ?? 'gm3';

      // The GM3 module sometimes use the shortcut of passing
      // an array of library names. If the item is not an array,
      // presumably it's a library name.
      if(!is_array($lib_settings)){
        unset($libraries[$library]);
        $library = $lib_settings;
        $libraries[$library] = $library;
      }

      // gm3 libs need gm3 prepended to their filename
      $library = ($module == 'gm3') ? "gm3.$library" : $library;
      drupal_add_library($module, $library);
    }
  }

  // Add the core library and the settings
  drupal_add_js(array(
    'gm3' => array(
      'maps' => array(
        $id => [
          'id' => $id,
          'settings' => $settings,
          'libraries' => $libraries
        ]
      )
    )
  ), 'setting');
}

/**
 * Theme a GM3 view.
 */
function theme_gm3_view_gm3($variables){
  static $colours = 0;
  $colours++;
  //$colours = $colours % 8;
  $geo_fields = array();
  $none_geo_fields = array();
  foreach($variables['view']->field as $field_key => $field){
    if(isset($field->field_info['type']) && substr($field->field_info['type'], 0, 4) == 'gm3_'){
      $geo_fields[$field_key] = $field->field_info['type'];
    }else{
      $none_geo_fields[] = $field_key;
    }
  }
  $map = array(
    'libraries' => array()
  );
  // Because it's not possible to click on two points with the exact same
  // location, we group content that have the exact same location.
  $array_points = array();
  foreach(array_keys($variables['rows']) as $index){
    $row = $variables['view']->result[$index];
    $content = $variables['rows'][$index];
    foreach($geo_fields as $key => $type){
      switch($type){
        case 'gm3_rectangle':
        case 'gm3_polygon':
        case 'gm3_polyline':
          foreach($row->{"field_$key"} as $item){
            module_load_include('functions.inc', 'gm3');
            if(!isset($map['libraries'][substr($type, 4)])){
              $map['libraries'][substr($type, 4)] = array(
                substr($type, 4) . 's' => array()
              );
            }
            $map['libraries'][substr($type, 4)][substr($type, 4) . 's'][] = array(
              substr($type, 4) => array_pop(gm3_convert_polygon_string($item['raw'][substr($type, 4)])),
              'editable' => FALSE,
              'content' => $content
            );
          }
          break;
        case 'gm3_point':
          if(!isset($map['libraries']['point'])){
            $map['libraries']['point'] = array(
              'points' => array()
            );
          }
          if(count($row->{"field_$key"})){
            foreach($row->{"field_$key"} as $item){
              $point = $item['raw'];
              if(isset($array_points[$point['latitude']][$point['longitude']])){
                if(!is_array($map['libraries']['point']['points'][$array_points[$point['latitude']][$point['longitude']]]['content'])){
                  $map['libraries']['point']['points'][$array_points[$point['latitude']][$point['longitude']]]['content'] = array(
                    array(
                      'title' => '1',
                      'content' => $map['libraries']['point']['points'][$array_points[$point['latitude']][$point['longitude']]]['content']
                    )
                  );
                }
                $map['libraries']['point']['points'][$array_points[$point['latitude']][$point['longitude']]]['content'][] = array(
                  'title' => count($map['libraries']['point']['points'][$array_points[$point['latitude']][$point['longitude']]]['content']) + 1,
                  'content' => $content
                );
              }else{
                $point['content'] = $content;
                $array_points[$point['latitude']][$point['longitude']] = count($map['libraries']['point']['points']);
                $map['libraries']['point']['points'][] = $point;
              }
            }
          }else{
            if(isset($row->{"field_data_{$key}_{$key}_latitude"})){
              $map['libraries']['point']['points'][] = array(
                'latitude' => $row->{"field_data_{$key}_{$key}_latitude"},
                'longitude' => $row->{"field_data_{$key}_{$key}_longitude"},
                'content' => $content
              );
            }
          }
          foreach(array_keys($map['libraries']['point']['points']) as $key){
            $map['libraries']['point']['points'][$key]['colour'] = $colours;
          }
          break;
        case 'gm3_combination':
          foreach($row->_field_data as $id => $values){
            if(empty($values['entity']->{$key})){
              continue;
            }
            foreach(entity_get_info() as $entity_type => $values2){
              if($values2['base table'] == $variables['view']->base_table){
                break;
              }
            }
            $lang = field_language($entity_type, $values['entity'], $key);
            foreach($values['entity']->{$key}[$lang] as $item){
              switch($item['gm3_type']){
                case 'point':
                  if(!isset($map['libraries']['point'])){
                    $map['libraries']['point'] = array(
                      'points' => array()
                    );
                  }
                  $map['libraries']['point']['points'][] = array(
                    'latitude' => $item['latitude'],
                    'longitude' => $item['longitude'],
                    'content' => $content
                  );
                  break;
                case 'rectangle':
                case 'polyline':
                case 'polygon':
                  module_load_include('functions.inc', 'gm3');
                  if(!isset($map['libraries'][$item['gm3_type']])){
                    $map['libraries'][$item['gm3_type']] = array(
                      $item['gm3_type'] . 's' => array()
                    );
                  }
                  $map['libraries'][$item['gm3_type']][$item['gm3_type'] . 's'][] = array(
                    $item['gm3_type'] => array_pop(gm3_convert_polygon_string($item[$item['gm3_type']])),
                    'editable' => FALSE,
                    'content' => $content
                  );
                  break;
                default:
                  // The default - This field type is not defined by the gm3
                  // module,
                  // we therefore append call the map_alter function for this
                  // field
                  $function = "{$item['gm3_type']}_map_alter";
                  if(function_exists($function)){
                    $function($map, $item);
                  }
                  break;
              }
            }
          }
          break;
        default:
          // The default - This field type is not defined by the gm3 module,
          // we therefore append call the map_alter function for this
          // field
          foreach($row->{"field_$key"} as $item){
            $function = "{$item['gm3_type']}_map_alter";
            if(function_exists($function)){
              $function($map, $item);
            }
          }
          break;
      }
    }
  }
  return theme('gm3_map', $map);
}
