<?php

/**
 * @file
 * This file describes the MM plugin as implemented by FICS
 */

/**
 * Plugin definition with callbacks
 */
$plugin = array(
  'description' => t('FICS ESDH MM Backend'),
  'version' => 1,
  'test' => 'os2web_fics_esdh_mm_test',
  'get_import_list' => 'os2web_fics_esdh_mm_get_import_list',
  'import_meeting' => 'os2web_fics_esdh_mm_import_meeting',
  'post_import_process' => '_os2web_fics_esdh_mm_post_import_process',
);

/**
 * For plugin testing purposes.
 */
function os2web_fics_esdh_mm_test($stuff) {
  return 'Test successful. Recieved: ' . $stuff;
}

/**
 * Returns a list of meeting_ids for import. In our case a list of uri's of xml files.
 *
 * @return array
 *   List of URIs
 */
function os2web_fics_esdh_mm_get_import_list() {
  /*
   * Protecting XML files in the import folder.
   */
  _os2web_fics_esdh_mm_ensure_htaccess();

  // Clearing variable with processed XML files
  variable_set('os2web_fics_esdh_mm_processed_xml_files', array());

  /*
   * For the FICS service, we need to import meetings from XML files.
   * We will enumerate the list with the publication-id
   * as parsed from the dir name of the manifest.
   */
  $uris = array();
  foreach (file_scan_directory(variable_get('os2web_fics_esdh_mm_path', MM_IMPORT_DIR), '/\.xml$/i', array('recurse' => TRUE)) as $xml_file) {
    $uris[] = array('id' => dirname($xml_file->uri), 'uri' => $xml_file->uri);
  }
  return $uris;
}


/**
 * Imports a meeting, and return the import as a structured array.
 *
 * @param object $meeting
 *   Meeting XML object.
 *
 * @return array
 *   List of meeting_structures to save
 */
function os2web_fics_esdh_mm_import_meeting($meeting) {
  $file = $meeting['uri'];
  $meetings = array();
  if (is_file(drupal_realpath($file))) {
    libxml_use_internal_errors(TRUE);

    $manifest = simplexml_load_string(file_get_contents(drupal_realpath($file)), 'SimpleXMLElement', LIBXML_NOWARNING);

    libxml_clear_errors();
    libxml_use_internal_errors(FALSE);

    if ($manifest === FALSE) {
       error_log("Failed loading XML");
       foreach(libxml_get_errors() as $error) {
          error_log ($error->message);
       }
    }

    if (is_object($manifest)) {

      // Setting the XML file to load - don't think we need this anymore.
      $directory_name = dirname(drupal_realpath($file));

      // Collecting agenda data
      $committee_id = (int) $manifest->BasisData->CommitteeID;
      $committee_name = (string) $manifest->BasisData->CommitteeName;
      $meeting_date = (string) $manifest->Publish->MeetingDate . ' ' . $manifest->Publish->MeetingBegin;
      $meeting_place =  (string)$manifest->Publish->MeetingPlace;
      $meeting_id = (int) $manifest->Publish->MeetingID;
      $meeting_state = '';
      $meeting_type = $manifest->Publish->Summary;

      // Collecting agenda items
      $meeting_agendas = $manifest->Publish->MetaData->Items;
      
      // Check if the committee is allowed to publish.
      $publish_committee = array_map('trim', explode(',', variable_get('os2web_fics_esdh_approved_committees', MM_DEFAULT_APPROVED)));
      if ($committee_name == '' || !in_array($committee_id, $publish_committee)) {
        watchdog('FICS MM', 'Ignored agenda from "%committee" .', array('%committee' => $committee_name));
        return FALSE;
      }

      // Fix dates.
      $date = new DateTime($meeting_date, new DateTimeZone('Europe/Copenhagen'));
      $title_date = $date->format('d-m-Y k\l. H:i');
      $date->setTimezone(new DateTimeZone('UTC'));
      $meeting_date = $date->format('Y-m-d H:i');

      // Changing the meeting state to match ESDH Provider module
      if($meeting_type == 'false') {
        $meeting_type = "Dagsorden";
      }
      else {
        $meeting_type = "Referat";
      }

      // Saving the meeting data in ESDH Provider format.
      $meeting = array(
        'system_id' => $meeting_id,
        'meeting_id' => $meeting_id,
        'meeting_sub_id' => $meeting_id,
        'committee' => $committee_name,
        'committee_id' => $committee_id,
        'location' => $meeting_place,
        'title' => $committee_name . ': ' . $title_date,
        'meeting_date_start' => $meeting_date,
        'type' => $meeting_type,
        'meeting_state' => $meeting_state,
      );

      // Error logging to watchdog if the agenda item list is empty
      if (empty($meeting_agendas)) {
        watchdog('FICS MM', 'Empty list of import items in %file.', array('%file' => $file), WATCHDOG_WARNING);
      }

      foreach ($meeting_agendas as $meeting_agenda) {
        // Importing each agenda item through the ESDH provider module
        $imported = _os2web_fics_esdh_mm_import_meeting_agenda($meeting, $meeting_agenda);
        if (is_array($imported)) {
          $imported['publish'] = TRUE;
          $meetings[] = $imported;
        }
      }
    }
    else {
      // Error logging to watchdog if the XML failde to parse. Check if all illegal characters are in the remove list. 
      watchdog('FICS MM', 'Failed to parse XML in %file during import.', array('%file' => $file), WATCHDOG_WARNING);
    }

    $processed_files = variable_get('os2web_fics_esdh_mm_processed_xml_files', array());
    $processed_files[] = $file;
    variable_set('os2web_fics_esdh_mm_processed_xml_files', $processed_files);
  }
  return $meetings;
}

/**
 * Imports a meeting from filesystem.
 *
 * @param array $meeting
 *   Meeting array.
 *
 * @param SimpleXMLElement $agenda_xml
 *   Meeting agenda XML object.
 *
 * @return mixed
 *   Meeting_struct array for an accepted import, else FALSE
 */
function _os2web_fics_esdh_mm_import_meeting_agenda(&$meeting, $agenda_xml) {
  $meeting['items'] = array();
  $agenda = $meeting;
  // Import all agenda items.
  $agenda_bullet = $agenda_xml;
  foreach ($agenda_bullet->Item as $agenda_item) {
    if (($item = _os2web_fics_esdh_mm_import_meeting_agenda_item($meeting, $agenda_item)) !== FALSE) {
      $agenda['items'][] = $item;
    }
  }
  // Apply defaults, just in case we missed something.
  $agenda += os2web_esdh_provider_default_meeting();
  return $agenda;
}

/**
 * Imports a single item for an agenda.
 *
 * @param array $meeting
 *   Meeting array.
 *
 * @param SimpleXMLElement $xml
 *   Xml segment containing the item
 *
 * @return array
 *   The imported item.
 */
function _os2web_fics_esdh_mm_import_meeting_agenda_item($meeting, $xml) {
  $item = array(
    'id' => (int) $xml->ItemNo,
    'title' => 'Pkt. ' . (int) $xml->ItemNo . ' ' . (string) $xml->DocTitle,
    'access' => ((string) $xml->CaseClosed == 'false') ? MM_OPEN_STRING : MM_CLOSED_STRING,
    'bullets' => array(),
    'enclosures' => array(),
    'doc' => array(),
    'bullets' => array(),
  );

  // Deny closed content.
  if ($item['access'] === MM_OPEN_STRING) {

    // Adding bullet title and content
    $bullet_content = (string) $xml->DocContent;
    $bullet_content = !empty($bullet_content) ? _os2web_fics_esdh_mm_extract_case_content($bullet_content) : t('No content');
    $item['bullets'][] = array(
      'title' => (string) $xml->DocTitle,
      'body' => $bullet_content
    );

    // Process attachments.
    foreach ($xml->CaseAttachments->Attachment as $attachment) {
      $attachment_item['filename'] = (string) $attachment->AttachmentFilename;
      $attachment_item['access'] = ((string) $attachment->AttachmentClosed == 'false') ? MM_OPEN_STRING : MM_CLOSED_STRING;
      $file_uri_parts = array(variable_get('os2web_fics_esdh_mm_path', MM_IMPORT_DIR), $meeting['meeting_id'], str_replace('\\', '/', $attachment_item['filename']));
      $file_uri = implode('/', $file_uri_parts);
      if (strlen($attachment_item['filename']) > 0 && file_exists($file_uri)) {
        if ($attachment_item['access'] == MM_OPEN_STRING) {
          $item['enclosures'][] = array(
            'uri' => $file_uri,
            'title' => trim((string) $attachment->AttachmentTitle),
            'access' => TRUE,
          );
        }
        else {
          _os2web_fics_esdh_mm_delete_attachments($file_uri, $meeting['meeting_id'] . '/' . $item['id']);
        }
      }
    }
  }
  else if (MM_DELETE_FILES_FOR_CLOSED) {
    // Delete the attachments
    foreach ($xml->CaseAttachments->Attachment as $attachment) {
      $attachment_item['filename'] = (string) $attachment->AttachmentFilename;
      $file_uri_parts = array(variable_get('os2web_fics_esdh_mm_path', MM_IMPORT_DIR), $meeting['meeting_id'], str_replace('\\', '/', $attachment_item['filename']));
      $file_uri = implode('/', $file_uri_parts);
      _os2web_fics_esdh_mm_delete_attachments($file_uri, $meeting['meeting_id'] . '/' . $item['id']);
    }
  }

  $item += os2web_esdh_provider_default_item();
  return $item;
}

/**
 * Extracts case content from the base64 encoded HTML document.
 *
 * @param string $content
 *   Base64 encoded HTML document.
 *
 * @return string
 *   The extracted HTML body content.
 */
function _os2web_fics_esdh_mm_extract_case_content($content) {
  $html = base64_decode($content);
  preg_match('/(?:<body[^>]*>)(.*)<\/body>/isU', $html, $matches);
  return $matches[1];
}

/**
 * Post import process callback.
 *
 * In the FICS MM plugin is it used to delete imported XML files,
 * because these can contain closed bullets.
 */
function _os2web_fics_esdh_mm_post_import_process($drush = FALSE) {
  if (!MM_KEEP_XML_FILES) {
    $processed_files = variable_get('os2web_fics_esdh_mm_processed_xml_files', array());
    if (is_array($processed_files) && count($processed_files) > 0) {
      foreach ($processed_files as $file) {
        $variables = array('%file' => $file);
        if (file_unmanaged_delete($file)) {
          watchdog('FICS MM', 'Successfully deleted XML %file after import.', $variables, WATCHDOG_INFO);
        }
        else {
          watchdog('FICS MM', 'Failed to delete XML %file after import.', $variables, WATCHDOG_ERROR);
        }
      }
    }
    variable_set('os2web_fics_esdh_mm_processed_xml_files', array());
  }
}

/**
 *
 * Deleting the specified file from the file system and logging the
 * result in watchdog.
 *
 * @param string $file
 *   Path to the file that should be deleted.
 *
 * @param string $meeting
 *   A string with meeting id and item id identifying the item we
 *   are deleting the file for. Only used for logging in the watchdog.
 */
function _os2web_fics_esdh_mm_delete_attachments($file, $meeting) {
  $variables = array('%file' => $file, '%meeting' => $meeting);

  if (file_exists($file)) {
    if (file_unmanaged_delete($file)) {
      watchdog('FICS MM', 'Deleted attachment %file during import with success. Meeting/item identification: %meeting.', $variables, WATCHDOG_INFO);
    }
    else {
      watchdog('FICS MM', 'Failed to delete attachment %file during import. Meeting/item identification: %meeting.', $variables, WATCHDOG_ERROR);
    }
  }
}

/**
 * Creates a .htaccess file in the FICS files folder which restricts
 * access to .XML files.
 */
function _os2web_fics_esdh_mm_ensure_htaccess() {
  $directory = variable_get('os2web_fics_esdh_mm_path', MM_IMPORT_DIR);
  if (file_uri_scheme($directory)) {
    $directory = file_stream_wrapper_uri_normalize($directory);
  }
  else {
    $directory = rtrim($directory, '/\\');
  }
  $htaccess_path =  $directory . '/.htaccess';

  if (file_exists($htaccess_path)) {
    // Short circuit if the .htaccess file already exists.
    return;
  }

  // Public .htaccess file.
  $htaccess_lines = "SetHandler Drupal_Security_Do_Not_Remove_See_SA_2006_006\nOptions None\nOptions +FollowSymLinks\n<FilesMatch \"\.(xml)$\">\nOrder allow,deny\n</FilesMatch>";

  // Write the .htaccess file.
  if (file_put_contents($htaccess_path, $htaccess_lines)) {
    drupal_chmod($htaccess_path, 0444);
  }
  else {
    $variables = array('%directory' => $directory, '!htaccess' => '<br />' . nl2br(check_plain($htaccess_lines)));
    watchdog('security', "Security warning: Couldn't write .htaccess file. Please create a .htaccess file in your %directory directory which contains the following lines: <code>!htaccess</code>", $variables, WATCHDOG_ERROR);
  }
}