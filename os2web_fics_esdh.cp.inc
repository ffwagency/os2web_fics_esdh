<?php

/**
 * @file
 * Functions related to the CP import fromFICS ESDH.
 */

/**
 * Reading import directory for XML files.
 */
function os2web_fics_esdh_cp_read_dir() {
  $uris = array();
  $path = variable_get('os2web_fics_esdh_cp_path', CP_IMPORT_DIR);
  foreach (file_scan_directory($path, '/\.xml$/i') as $xml_file) {
    $uris[] = $xml_file->uri;
  }
  return $uris;
}

/**
 * Imports a case and document.
 *
 * @param string $xml_file
 *   XML file URI.
 *
 * @return bool
 *   Returns TRUE on succesfull import.
 */
function _os2web_esdh_fics_cp_import_document($xml_file) {
  if (is_file(drupal_realpath($xml_file))) {
    libxml_use_internal_errors(TRUE);

    $manifest = simplexml_load_string(file_get_contents(drupal_realpath($xml_file)), 'SimpleXMLElement', LIBXML_NOWARNING);

    libxml_clear_errors();
    libxml_use_internal_errors(FALSE);

    if ($manifest === FALSE) {
      error_log("Failed loading XML");
      foreach(libxml_get_errors() as $error) {
        error_log($error->message);
      }
    }

    if (is_object($manifest)) {
      $case_number = (int) $manifest->sagsnr;
      $record_number = (string) $manifest->journalnr;
      $case_title = (string) $manifest->sagsnavn1;
      $document_number = (int) $manifest->brevid;
      $document_date = (string) $manifest->dokumentdato;
      $document_title = (string) $manifest->dokumenttitel;
      $document_file = (string) $manifest->dokref;
      $document_file = substr($document_file, strrpos($document_file, "\\") + 1);

      // Checking if the document already exists in Drupal
      if ($document_nid = _os2web_esdh_fics_cp_check_document($document_number)) {
        // Loading existing document node
        $document_node = node_load($document_nid, NULL, TRUE);
      }
      else {
        // Creating new node object
        $document_node = new stdClass();
        $document_node->type = "os2web_case_publishing_document";
        $document_node->language = LANGUAGE_NONE;
        $document_node->uid = 1;
        node_object_prepare($document_node);
      }

      // Saving file
      if (!empty($document_file)) {
        $file_source_uri = variable_get('os2web_fics_esdh_cp_path', CP_IMPORT_DIR);
        $file_source_uri .= '/' . $document_file;
        $file_destination_uri = variable_get('os2web_fics_esdh_cp_destination_path', CP_DESTINATION_DIR);
        $file_destination_uri .= '/' . $document_file;

        // Saving file if it exists
        if (file_exists($file_source_uri)) {
          $file_id = db_select('file_managed', 'fm')
            ->fields('fm', array('fid'))
            ->condition('uri', $file_destination_uri)
            ->execute()->fetchField();

          $file = new stdClass();
          if ($file_id) {
            $file->fid = $file_id;
          }
          $file->uri = $file_source_uri;
          $file->display = 1;
          $file->filename = drupal_basename($file->uri);
          $file->filemime = file_get_mimetype($file->uri);
          $file->uid = 1;
          $file->status = FILE_STATUS_PERMANENT;
          $file = file_save($file);

          // Moving file final destination
          file_move($file, $file_destination_uri, FILE_EXISTS_REPLACE);
        }
      }

      // Updating the document node object and saving the node.
      $document_node->published = 1;
      $document_node->title = 'Dokument-' . $document_number . ' - ' . $document_title . ' (' . $record_number . ')';
      $document_node->field_os2web_case_document_id[LANGUAGE_NONE][0]['value'] = $document_number;
      $document_node->field_os2web_case_document_name[LANGUAGE_NONE][0]['value'] = $document_title;
      $document_node->field_os2web_case_document_date[LANGUAGE_NONE][0]['value'] = $document_date;
      $document_node->field_os2web_case_document_date[LANGUAGE_NONE][0]['timezone'] = 'Europe/Berlin';
      $document_node->field_os2web_case_document_date[LANGUAGE_NONE][0]['date_type'] = 'datetime';
      if (isset($file) && is_object($file)) {
        $document_node->field_os2web_case_document_file[LANGUAGE_NONE][] = (array) $file;
      }
      node_save($document_node);

      // Checking if the case already exists in Drupal
      if ($case_nid = _os2web_esdh_fics_cp_check_case($case_number)) {
        // Loading existing case node
        $case_node = node_load($case_nid, NULL, TRUE);
      }
      else {
        // Creating new node object
        $case_node = new stdClass();
        $case_node->type = "os2web_case_publishing_case";
        $case_node->language = LANGUAGE_NONE;
        $case_node->uid = 1;
        node_object_prepare($case_node);
      }

      // Updating the case node object and saving the node.
      $case_node->published = 1;
      $case_node->title = 'Sag-' . $case_number . ' - ' . $case_title . ' (' . $record_number . ')';
      $case_node->field_os2web_case_case_id[LANGUAGE_NONE][0]['value'] = $case_number;
      $case_node->field_os2web_case_record_number[LANGUAGE_NONE][0]['value'] = $record_number;
      if (!_os2web_esdh_fics_cp_check_relation($case_node, $document_node->nid)) {
        if (isset($case_node->field_os2web_case_documents) && is_array($case_node->field_os2web_case_documents[LANGUAGE_NONE])) {
          $document_refs = $case_node->field_os2web_case_documents[LANGUAGE_NONE];
        }
        else {
          $document_refs = array();
        }
        $document_refs[]['target_id'] = $document_node->nid;
        $case_node->field_os2web_case_documents[LANGUAGE_NONE] = $document_refs;
      }
      node_save($case_node);

      // Delete XML file
      if (!CP_KEEP_XML_FILES) {
        file_unmanaged_delete($xml_file);
      }

      // Logging that the file was imported.
      watchdog('FICS CP', 'XML file %file was import correct.', array('%file' => $xml_file), WATCHDOG_INFO);
      variable_set('os2web_fics_esdh_cp_last_import', time());
    }
    else {
      // Error logging to watchdog if the XML failed to parse.
      watchdog('FICS CP', 'Failed to parse XML in %file during import.', array('%file' => $xml_file), WATCHDOG_WARNING);
    }
  }
  return TRUE;
}

/**
 * Checking if a given document already exists in Drupal.
 *
 * @param int $number
 *   The document number.
 *
 * @return int
 *   The found node ID or 0.
 */
function _os2web_esdh_fics_cp_check_document($number) {
  $query = new EntityFieldQuery();
  $result = $query->entityCondition('entity_type', 'node')
    ->propertyCondition('type', 'os2web_case_publishing_document')
    ->fieldCondition('field_os2web_case_document_id', 'value', $number, '=')
    ->execute();
  $nid = 0;
  if (isset($result['node'])) {
    $nid = array_shift($result['node'])->nid;
  }
  return $nid;
}

/**
 * Checking if a given case already exists in Drupal.
 *
 * @param int $number
 *   The case number.
 *
 * @return int
 *   The found node ID or 0.
 */
function _os2web_esdh_fics_cp_check_case($number) {
  $query = new EntityFieldQuery();
  $result = $query->entityCondition('entity_type', 'node')
    ->propertyCondition('type', 'os2web_case_publishing_case')
    ->fieldCondition('field_os2web_case_case_id', 'value', $number, '=')
    ->execute();
  $nid = 0;
  if (isset($result['node'])) {
    $nid = array_shift($result['node'])->nid;
  }
  return $nid;
}

/**
 * Checking if a given document ID is already related to the case.
 *
 * @param object $case
 *   The full case object.
 *
 * @param int $document_id
 *   The document node ID.
 *
 * @return bool
 *   Return TRUE on exists else FALSE.
 */
function _os2web_esdh_fics_cp_check_relation($case, $document_id) {
  if (is_object($case)) {
    if (isset($case->field_os2web_case_documents)) {
      foreach ($case->field_os2web_case_documents[LANGUAGE_NONE] as $document) {
        if ((int) $document['target_id'] == $document_id) {
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}