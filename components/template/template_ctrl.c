/**
 * @file eth.c
 * @author A.Czerwinski@pistacje.net
 * @brief Template Controller
 * @version 0.1
 * @date 2025-01-31
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "msg.h"
#include "template_ctrl.h"


static const char* TAG = "EWHC::COMPONENT:TEMPLATE";

/**
 * @brief Init Template controller
 * 
 * \return esp_err_t 
 */
esp_err_t TemplateCtrl_Init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done Template controller
 * 
 * \return esp_err_t 
 */
esp_err_t TemplateCtrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run Template controller
 * 
 * \return esp_err_t 
 */
esp_err_t TemplateCtrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Sent message to the Template controller thread
 * 
 * \return esp_err_t 
 */
esp_err_t TemplateCtrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
