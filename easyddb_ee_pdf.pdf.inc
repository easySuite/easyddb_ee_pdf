<?php

/**
 * @file
 * PDF generation form logic.
 */

/**
 * Page callback for 'easyddb_ee_pdf'.
 */
function easyddb_ee_pdf_generate_pdf_form(array $form, array &$form_state): array {
  $form['ee_pdf_title'] = [
    '#type' => 'textfield',
    '#title' => t('PDF title'),
    '#default_value' => variable_get('ee_pdf_title', ''),
  ];

  $ee_pdf_intro = variable_get('ee_pdf_intro', []);
  $form['ee_pdf_intro'] = [
    '#type' => 'text_format',
    '#format' => $ee_pdf_intro['format'] ?? 'ding_wysiwyg',
    '#title' => t('Welcome text'),
    '#description' => t('Welcome text for the 2nd page of the PDF file.'),
    '#default_value' => $ee_pdf_intro['value'] ?? '',
  ];

  $form['ee_pdf_image'] = [
    '#title' => t('Cover'),
    '#type' => 'media',
    '#media_options' => array(
      'global' => array(
        'types' => array('image'),
      ),
    ),
    '#default_value' => variable_get('ee_pdf_image', NULL),
  ];

  $form['submit'] = [
    '#type' => 'submit',
    '#value' => t('Generate PDF'),
  ];

  $form['#submit'][] = 'system_settings_form_submit';
  $form['#submit'][] = 'easyddb_ee_pdf_generate_pdf_form_submit';

  return $form;
}

function easyddb_ee_pdf_generate_pdf_form_submit(array $form, array &$form_state) {
  form_state_values_clean($form_state);
  $values = $form_state['values'];

  require_once drupal_get_path('module', 'easyddb_ee_pdf') . '/lib/vendor/autoload.php';



  $mpdf = new Mpdf\Mpdf();
  $mpdf->SetHTMLFooter(theme(
    'ee_pdf_footer',
    [
      'title' => $values['ee_pdf_title'] ?? ''
    ]
  ));

  $events_by_category = easyddb_ee_pdf_load_events(
    (new \DateTime())->format('Y'),
    'kl'
  );
  $events_markup = '';
  foreach ($events_by_category as $category_tid => $events) {
    $category_term = taxonomy_term_load($category_tid);

    $mpdf->SetHTMLHeader(theme(
      'ee_pdf_header',
      [
        'category_term' => $category_term,
      ]
    ));
    $mpdf->AddPage();
    foreach ($events as $event) {
      $events_markup .= theme('ee_pdf_event', ['event' => $event]);
    }

    $mpdf->WriteHTML($events_markup);
  }

  $mpdf->Output();

  drupal_exit();
}

function easyddb_ee_pdf_load_events($year, $language): array{
  $query = new EntityFieldQuery();

  $query->entityCondition('entity_type', 'node')
    ->propertyCondition('status', 1)
    ->propertyCondition('type', 'ding_event')
    ->propertyCondition('language', $language)
    ->fieldCondition('field_ding_event_date', 'value', $year, 'STARTS_WITH')
    ->propertyOrderBy('created', 'DESC')
    ->range(0,10);

  $result = $query->execute();

  $nodes = node_load_multiple(array_keys($result['node']));
  $result = [];
  foreach ($nodes as $node) {
    $category = field_get_items('node', $node, 'field_ding_event_category');
    $category_tid = $category[0]['tid'] ?? '';

    $result[$category_tid][] = $node;
  }

  return $result;
}
