<?php

/**
 * @file
 * Functions related to the administration of the FICS ESDH
 * implementation.
 */

/**
 * Page callback. Displays CP import page.
 */
function _os2web_fics_esdh_cp_import() {
  $form = drupal_get_form('os2web_fics_esdh_cp_import_form');
  return drupal_render($form);
}

/**
 * The CP import form.
 */
function os2web_fics_esdh_cp_import_form() {
  $form = array();
  $form['info'] = array(
    '#type' => 'markup',
    '#markup' => t('Import of cases and documents from FICS ESDH.'),
  );
  $form['status'] = array(
    '#type' => 'item',
    '#title' => t('Status'),
    '#markup' => t('Last import was %time.', array('%time' => format_date(variable_get('os2web_fics_esdh_cp_last_import', 0)))),
  );
  $form['import'] = array(
    '#type' => 'submit',
    '#value' => t('Run import'),
  );
  return $form;
}

/**
 * Submit callback for the CP import form.
 */
function os2web_fics_esdh_cp_import_form_submit($form, &$form_state) {
  drupal_goto('admin/config/os2web/fics_esdh_cp/import-confirm');
  return;
}

/**
 * The CP import confirm form.
 */
function _os2web_fics_esdh_cp_import_confirm($form, &$form_state) {
  $msg = t('Are you sure you want to run this import?');
  return confirm_form($form, t('Import cases and documents'), isset($_GET['destination']) ? $_GET['destination'] : "admin/config/os2web/fics_esdh_cp", $msg, t('Run'), t('Cancel'));
}

/**
 * Submit callback for the CP import confirm form.
 */
function _os2web_fics_esdh_cp_import_confirm_submit($form, &$form_state) {
  module_load_include('inc', 'os2web_fics_esdh', 'os2web_fics_esdh.cp');

  $batch = array(
    'title' => t('Case and document import'),
    'operations' => array(),
    'file' => drupal_get_path('module', 'os2web_fics_esdh') . '/os2web_fics_esdh.cp.inc',
    'init_message' => t('Checking for XML files with cases....'),
  );
  $files = os2web_fics_esdh_cp_read_dir();
  foreach ($files as $file) {
    $batch['operations'][] = array('_os2web_esdh_fics_cp_import_document',array($file));
  }
  batch_set($batch);
  batch_process('admin/config/os2web/fics_esdh_cp');
}
