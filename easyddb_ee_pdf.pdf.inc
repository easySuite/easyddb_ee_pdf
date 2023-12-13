<?php

/**
 * @file
 * PDF generation form logic.
 */

use Mpdf\HTMLParserMode;
use Mpdf\Mpdf;

/**
 * Page callback for 'easyddb_ee_pdf'.
 */
function easyddb_ee_pdf_generate_pdf_form(array $form, array &$form_state): array {
  $form['ee_pdf_image'] = [
    '#title' => t('Main cover'),
    '#type' => 'media',
    '#media_options' => array(
      'global' => array(
        'types' => array('image'),
      ),
    ),
    '#default_value' => variable_get('ee_pdf_image', NULL),
  ];

  $form['ee_pdf_title'] = [
    '#type' => 'textfield',
    '#title' => t('Welcome title'),
    '#description' => t('Welcome title for the 2nd page of the PDF file.'),
    '#default_value' => variable_get('ee_pdf_title', []),
  ];

  $ee_pdf_intro = variable_get('ee_pdf_intro', []);
  $form['ee_pdf_intro'] = [
    '#type' => 'text_format',
    '#format' => $ee_pdf_intro['format'] ?? 'ding_wysiwyg',
    '#title' => t('Welcome text'),
    '#description' => t('Welcome text for the 2nd page of the PDF file.'),
    '#default_value' => $ee_pdf_intro['value'] ?? '',
  ];

  $form['ee_pdf_toc_header'] = [
    '#type' => 'textfield',
    '#title' => t('Table of contents header'),
    '#default_value' => variable_get('ee_pdf_toc_header', ''),
  ];

  $form['ee_pdf_toc_prefix'] = [
    '#type' => 'textfield',
    '#title' => t('Table of contents prefix'),
    '#default_value' => variable_get('ee_pdf_toc_prefix', ''),
  ];

  $form['ee_pdf_footer'] = [
    '#type' => 'textfield',
    '#title' => t('Footer text'),
    '#default_value' => variable_get('ee_pdf_footer', ''),
  ];

  $languages = language_list();
  $form['ee_pdf_language'] = [
    '#type' => 'select',
    '#title' => t('Events language'),
    '#options' => array_combine(
      array_keys($languages),
      array_map(function ($v) {
        return $v->native;
      }, $languages),
    ),
    '#default_value' => variable_get('ee_pdf_language', ''),
  ];

  $form['submit'] = [
    '#type' => 'submit',
    '#value' => t('Generate PDF'),
  ];

  $form['#submit'][] = 'system_settings_form_submit';
  $form['#submit'][] = 'easyddb_ee_pdf_generate_pdf_form_submit';

  return $form;
}

/**
 * Custom submit handler for PDF generator form.
 *
 * Takes care to generate the PDF file.
 */
function easyddb_ee_pdf_generate_pdf_form_submit(array $form, array &$form_state) {
  form_state_values_clean($form_state);
  $values = $form_state['values'];

  require_once drupal_get_path('module', 'easyddb_ee_pdf') . '/lib/vendor/autoload.php';

  $mpdf = new Mpdf([
    'default_font' => 'sans-serif',
    'setAutoTopMargin' => 'stretch',
  ]);

  // 1st page.
  $image = file_load($values['ee_pdf_image'] ?? '');
  if ($image) {
    $mpdf->AddPageByArray([]);
    $mpdf->Image(
      drupal_realpath($image->uri),
      0,
      0,
      $image->width,
      $image->height,
      '',
      '',
      TRUE,
      FALSE
    );
  }

  // 2nd page.
  $mpdf->AddPageByArray([]);
  $mpdf->WriteHTML(theme('ee_pdf_intro', [
    'title' => $values['ee_pdf_title'],
    'intro' => $values['ee_pdf_intro']['value'] ?? '',
  ]), HTMLParserMode::HTML_BODY);

  $mpdf->SetHTMLFooter(theme('ee_pdf_footer', [
    'title' => ($values['ee_pdf_footer'] ?? ''),
  ]));

  // 3rd page - Table of Contents.
  $mpdf->TOCpagebreakByArray([
    'pages' => TRUE,
    'links' => TRUE,
    'toc_preHTML' => $values['ee_pdf_toc_prefix'] ?? '',
  ]);

  $events_by_category = easyddb_ee_pdf_load_events(
    (new \DateTime())->format('Y'),
    $values['ee_pdf_language'] ?? ''
  );

  end($events_by_category);
  $last_key = key($events_by_category);

  // N-page with events.
  foreach ($events_by_category as $category_tid => $events_by_library) {
    $category_term = taxonomy_term_load($category_tid);
    $mpdf->TOC_Entry($category_term->name);
    $mpdf->SetHTMLHeader(
      theme('ee_pdf_header', ['title' => $category_term->name]),
      '',
      TRUE
    );

    foreach ($events_by_library as $library_name => $events) {
      $mpdf->TOC_Entry($library_name, 1);
      $event_markup = theme('ee_pdf_library_events', [
        'library' => $library_name,
        'events' => $events,
      ]);

      $mpdf->WriteHTML($event_markup, HTMLParserMode::HTML_BODY);
    }

    // Do not add pages if it's last category in the loop.
    if ($category_tid !== $last_key) {
      $mpdf->AddPageByArray([]);
    }
  }

  // Suppress header for TOC as it inherits header from last page.
  $mpdf->SetHTMLHeader(theme('ee_pdf_header', [
    'title' => ($values['ee_pdf_toc_header'] ?? ''),
  ]));
  $mpdf->Output();

  drupal_exit();
}

/**
 * Loads and packs events.
 *
 * @param string $year
 *   Load events that start at a given year.
 * @param string $language
 *   Load events that use the given langcode.
 *
 * @return array
 *   A set of events packed as nested "category -> library -> node".
 */
function easyddb_ee_pdf_load_events($year, $language): array {
  $query = new EntityFieldQuery();

  $query->entityCondition('entity_type', 'node')
    ->propertyCondition('status', 1)
    ->propertyCondition('type', 'ding_event')
    ->propertyCondition('language', $language)
    ->fieldCondition('field_ding_event_date', 'value', $year, 'STARTS_WITH')
    ->propertyOrderBy('created', 'DESC')
    ->range(0, 10);

  $result = $query->execute();

  $nodes = node_load_multiple(array_keys($result['node']));
  $result = [];
  foreach ($nodes as $node) {
    $category = field_get_items('node', $node, 'field_ding_event_category');
    $category_tid = $category[0]['tid'] ?? '';
    $library_reference = field_get_items('node', $node, 'og_group_ref');
    $library_node = node_load($library_reference[0]['target_id'] ?? NULL);

    $result[$category_tid][$library_node->title ?? ''][] = $node;
  }

  // Sort the collection by library name.
  array_walk($result, function (&$v) {
    ksort($v);
  });

  return $result;
}
