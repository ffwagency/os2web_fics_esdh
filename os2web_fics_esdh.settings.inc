<?php
/**
 * @file
 * Settings for os2web_fics_esdh
 */

/**
 * Implements hook_form_alter().
 *
 * Add configuration to the settings form.
 */
function os2web_fics_esdh_form_os2web_settings_settings_form_alter(&$form, &$form_state) {
  // MM configuration.
  $form['meetings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Minutes import'),
    '#description' => t('Settings for the import of meeting agendas and minutes.'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['meetings']['os2web_fics_esdh_mm_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path to FICS publish folder'),
    '#description' => t('The path where FICS publish the XML-files to.'),
    '#default_value' => variable_get('os2web_fics_esdh_mm_path', MM_IMPORT_DIR),
  );
  $form['meetings']['os2web_fics_esdh_approved_committees'] = array(
    '#type' => 'textfield',
    '#title' => t('Approved committees'),
    '#maxlength' => 200,
    '#description' => t("Comma separated list of commitee ID's approved for publishing"),
    '#default_value' => variable_get('os2web_fics_esdh_approved_committees', MM_DEFAULT_APPROVED),
  );

  if (os2web_fics_esdh_cp_enabled()) {
    // Case publishing configuration.
    $form['cp'] = array(
      '#type' => 'fieldset',
      '#title' => t('Cases publishing import'),
      '#description' => t('Settings for the import of cases and documents.'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['cp']['os2web_fics_esdh_cp_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to FICS case publishing folder'),
      '#description' => t('The path where FICS publish the case XML-files.'),
      '#default_value' => variable_get('os2web_fics_esdh_cp_path', CP_IMPORT_DIR),
    );
    $form['cp']['os2web_fics_esdh_cp_destination_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to destination folder'),
      '#description' => t('The path to the folder where imported documents are stored.'),
      '#default_value' => variable_get('os2web_fics_esdh_cp_destination_path', CP_DESTINATION_DIR),
    );
  }
}
