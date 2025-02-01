/**
 * @file eth.c
 * @author A.Czerwinski@pistacje.net
 * @brief Ethernet Controller
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
#include "eth_ctrl.h"


static const char* TAG = "EWHC::COMPONENT:ETH";

/**
 * @brief Init Eth controller
 * 
 * \return esp_err_t 
 */
esp_err_t EthCtrl_Init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done Eth controller
 * 
 * \return esp_err_t 
 */
esp_err_t EthCtrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run Eth controller
 * 
 * \return esp_err_t 
 */
esp_err_t EthCtrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Sent message to the Eth controller thread
 * 
 * \return esp_err_t 
 */
esp_err_t EthCtrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
