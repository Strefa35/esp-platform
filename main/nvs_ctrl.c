/**
 * @file nvs_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief NVS Controller
 * @version 0.1
 * @date 2025-04-15
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <string.h>

#include "sdkconfig.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "nvs_ctrl.h"

#include "err.h"
#include "lut.h"


static const char* TAG = "ESP::NVM";


typedef struct nvs_s {
  nvs_handle_t nvs_handle;

} nvs_s;


/**
 * @brief Open NVS partition
 * 
 * \param partition - partition name
 * \param handle_ptr - pointer to nvs handle
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t NVS_Open(const char* partition, nvs_t* const handle_ptr) {
  nvs_t handle = NULL;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(partition: '%s', handle_ptr: %p)", __func__, partition ? partition : "---", handle_ptr);

  if (partition == NULL) {
    ESP_LOGE(TAG, "[%s] partition=NULL", __func__);
    return ESP_FAIL;
  }

  if (handle_ptr == NULL) {
    ESP_LOGE(TAG, "[%s] handle_ptr=NULL", __func__);
    return ESP_FAIL;
  }

  handle = malloc(sizeof(nvs_s));
  if (handle == NULL) {
    ESP_LOGE(TAG, "[%s] Memory allocation problem", __func__);
    result = ESP_ERR_NO_MEM;
  }

  memset(handle, 0x00, sizeof(nvs_s));

  ESP_LOGD(TAG, "nvs_open()");
  result = nvs_open(partition, NVS_READWRITE, &(handle->nvs_handle));
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] nvs_open() failed: %d.", __func__, result);
    free(handle);
    return result;
  }

  *handle_ptr = handle;
  
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Close NVS partition
 * 
 * \param handle - nvs handle
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t NVS_Close(nvs_t handle) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  if (handle->nvs_handle) {
    nvs_close(handle->nvs_handle);
  }
  free(handle);
  handle = NULL;
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Read data from NVS
 * 
 * \param handle - nvs handle
 * \param key - key name
 * \param data_ptr - pointer to data buffer
 * \param data_size - pointer to data size
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t NVS_Read(const nvs_t handle, const char *key, void* data_ptr, size_t* data_size) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, key: '%s')", __func__, handle, key);
  if (handle->nvs_handle) {
    result = nvs_get_blob(handle->nvs_handle, key, data_ptr, data_size);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Write data to NVS
 * 
 * \param handle - nvs handle
 * \param key - key name
 * \param data_ptr - pointer to data buffer
 * \param data_size - size of data
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t NVS_Write(const nvs_t handle, const char *key, void* data_ptr, size_t data_size) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, key: '%s', data_ptr: %p, data_size: %d)", __func__, 
        handle, key, data_ptr, data_size);
  if (handle->nvs_handle) {
    result = nvs_set_blob(handle->nvs_handle, key, data_ptr, data_size);
    if (result == ESP_OK) {
      result = nvs_commit(handle->nvs_handle);
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Init NVS
 * 
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t NVS_Init(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_MGR_CTRL_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);

  // Initialize NVS
  result = nvs_flash_init();
  if ((result == ESP_ERR_NVS_NO_FREE_PAGES) || (result == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      result = nvs_flash_init();
  }
  ESP_ERROR_CHECK(result);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done NVS
 * 
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t NVS_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
