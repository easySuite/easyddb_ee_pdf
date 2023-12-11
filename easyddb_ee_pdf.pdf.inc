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

  return $form;
}

function easyddb_ee_pdf_generate_pdf_form_submit(array $form, array &$form_state) {

}
