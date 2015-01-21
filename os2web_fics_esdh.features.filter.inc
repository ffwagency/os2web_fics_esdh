<?php
/**
 * @file
 * os2web_fics_esdh.features.filter.inc
 */

/**
 * Implements hook_filter_default_formats().
 */
function os2web_fics_esdh_filter_default_formats() {
  $formats = array();

  // Exported format: OS2Web FICS ESDH.
  $formats['os2web_fics_esdh'] = array(
    'format' => 'os2web_fics_esdh',
    'name' => 'OS2Web FICS ESDH',
    'cache' => 1,
    'status' => 1,
    'weight' => 0,
    'filters' => array(
      'filter_url' => array(
        'weight' => -50,
        'status' => 1,
        'settings' => array(
          'filter_url_length' => 72,
        ),
      ),
      'filter_html' => array(
        'weight' => -49,
        'status' => 1,
        'settings' => array(
          'allowed_html' => '<em> <strong> <cite> <blockquote> <code> <ul> <ol> <li> <dl> <dt> <dd> <table> <tr> <td> <h1> <h2> <h3>',
          'filter_html_help' => 1,
          'filter_html_nofollow' => 0,
        ),
      ),
      'filter_autop' => array(
        'weight' => -47,
        'status' => 1,
        'settings' => array(),
      ),
      'filter_htmlcorrector' => array(
        'weight' => -46,
        'status' => 1,
        'settings' => array(),
      ),
      'os2web_cpr_filter' => array(
        'weight' => 0,
        'status' => 1,
        'settings' => array(
          'modulus11_check' => 1,
          'date_check' => 1,
          'replace_all_dash' => 1,
          'dummy_value' => 'XXXXXX-XXXX',
        ),
      ),
    ),
  );

  return $formats;
}
