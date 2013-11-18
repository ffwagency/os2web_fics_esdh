<?php

/**
 * @file
 * This file describes the CM plugin as implemented by FICS.
 */

/**
 * Plugin definition with callbacks
 */
$plugin = array(
  'description' => t('FICS ESDH CM Backend'),
  'version' => 1,
  'test' => 'os2web_fics_esdh_cm_test',
  'get_document' => 'os2web_fics_esdh_cm_get_document',
  'get_document_file' => 'os2web_fics_esdh_cm_get_document_file',
  'get_case_docs' => 'os2web_fics_esdh_cm_get_case_docs',
  'get_case_data' => 'os2web_fics_esdh_cm_get_case_data',
  'lookup_doc' => 'os2web_fics_esdh_cm_lookup_doc',
  'lookup_case' => 'os2web_fics_esdh_cm_lookup_case',
);

/**
 * For plugin testing purposes.
 */
function os2web_fics_esdh_cm_test($stuff) {
  return 'Test successful. Received: ' . $stuff;
}

/**
 * This operation check for proper operation.
 *
 * Called often, so static cache is acceptable.
 *
 * @access public
 * @return bool
 *   True if the service is available.
 */
function os2web_fics_esdh_cm_available() {
  return TRUE;
}

function os2web_fics_esdh_cm_get_document() {

}

function os2web_fics_esdh_cm_get_document_file() {

}

function os2web_fics_esdh_cm_get_case_docs() {

}

function os2web_fics_esdh_cm_get_case_data() {

}

function os2web_fics_esdh_cm_lookup_doc() {

}

function os2web_fics_esdh_cm_lookup_case() {

}