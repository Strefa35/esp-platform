/**
 * @file mqtt_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief MQTT Controller with redundant configuration storage
 * @version 0.2
 * @date 2026-02-01
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 * Configuration stored in plain format with CRC32 validation.
 * 
 * NVS Partition Structure:
 * - Partition: "config"
 *   - Key: "mqtt-boot"  -> uint8_t (1 = mqtt-1, 2 = mqtt-2)
 *   - Key: "mqtt-1"     -> mqtt_config_t (config slot 1)
 *   - Key: "mqtt-2"     -> mqtt_config_t (config slot 2)
 */

#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cJSON.h"

#include "mqtt_client.h"

#include "sdkconfig.h"

#include "msg.h"
#include "nvs_ctrl.h"
#include "mgr_ctrl.h"
#include "mqtt_ctrl.h"
#include "tools.h"

#include "err.h"
#include "lut.h"
#include "mqtt_lut.h"


#define MQTT_TASK_NAME                "mqtt-task"
#define MQTT_TASK_STACK_SIZE          4096
#define MQTT_TASK_PRIORITY            9

#define MQTT_MSG_MAX                  10

#define MQTT_TOPIC_MAX_LEN            (32U)
#define MQTT_UID_LEN                  (10U)
#define MQTT_TOPIC_LEN                (MQTT_TOPIC_MAX_LEN - MQTT_UID_LEN - 1)

#define MQTT_UID_IDX                  (0U)
#define MQTT_TOPIC_IDX                (10U)

/* Temporary Broker URL - should be taken from NVS */
//#define CONFIG_BROKER_URL           "mqtt://10.0.0.10"
//#define CONFIG_BROKER_URL           "mqtt://10.0.0.45"
#define CONFIG_BROKER_URL             CONFIG_MQTT_CTRL_BROKER_URL
#define CONFIG_BROKER_PORT            CONFIG_MQTT_CTRL_BROKER_PORT
#define CONFIG_CREDENTIAL_USERNAME    CONFIG_MQTT_CTRL_CREDENTIAL_USERNAME
#define CONFIG_CREDENTIAL_PASSWORD    CONFIG_MQTT_CTRL_CREDENTIAL_PASSWORD

#define MQTT_NVS_NAME_SIZE            (20U)
#define MQTT_URI_SIZE                 (40U)
#define MQTT_USERNAME_SIZE            (32U)
#define MQTT_PASSWORD_SIZE            (64U)

/* NVS Configuration Partition and Keys */
#define MQTT_CONFIG_PARTITION         "config"
#define MQTT_CONFIG_BOOT_KEY          "mqtt-boot"
#define MQTT_CONFIG_SLOT1_KEY         "mqtt-1"
#define MQTT_CONFIG_SLOT2_KEY         "mqtt-2"

/* Configuration slot indices */
#define MQTT_CONFIG_SLOT_1            1U
#define MQTT_CONFIG_SLOT_2            2U

#define MQTT_NVS_FAILS_MAX            3

/* Configuration slot enumeration */
typedef enum {
  MQTT_SLOT_1 = 1,
  MQTT_SLOT_2,
  MQTT_SLOT_MAX
} mqtt_slot_e;

/* Helper macro to get passive slot (opposite of active) */
#define MQTT_GET_PASSIVE_SLOT(active) ((active) == MQTT_SLOT_1 ? MQTT_SLOT_2 : MQTT_SLOT_1)

static esp_mqtt_client_handle_t mqtt_client = NULL;

static QueueHandle_t      mqtt_msg_queue = NULL;
static TaskHandle_t       mqtt_task_id = NULL;
static SemaphoreHandle_t  mqtt_sem_id = NULL;

static data_uid_t         esp_uid = {0};
static data_eth_mac_t     esp_mac = {0};

/* NVS handle for "config" partition */
static nvs_t              mqtt_config_nvs_handle = NULL;

/* Currently active configuration slot (passive is calculated as opposite) */
static mqtt_slot_e        mqtt_slot = MQTT_SLOT_1;

/* Flag indicating config update in progress (waiting for successful reconnection) */
static bool               mqtt_config_update_in_progress = false;

typedef struct {
  uint8_t   fails;
  char      uri[MQTT_URI_SIZE];
  uint32_t  port;
  char      username[MQTT_USERNAME_SIZE];
  char      password[MQTT_PASSWORD_SIZE];
  uint32_t  crc;  /* CRC32 for data validation */
} mqtt_config_t;

/* Static buffers for mqtt_cfg to avoid dangling pointers */
static char mqtt_cfg_uri[MQTT_URI_SIZE] = {};
static char mqtt_cfg_username[MQTT_USERNAME_SIZE] = {};
static char mqtt_cfg_password[MQTT_PASSWORD_SIZE] = {};

esp_mqtt_client_config_t mqtt_cfg = {
  .broker.address.uri = mqtt_cfg_uri,
  .session.protocol_ver = MQTT_PROTOCOL_V_5,
  .network.disable_auto_reconnect = true,
  .credentials.username = mqtt_cfg_username,
  .credentials.authentication.password = mqtt_cfg_password,
  // .session.last_will.topic = "/topic/will",
  // .session.last_will.msg = "i will leave",
  // .session.last_will.msg_len = 12,
  // .session.last_will.qos = 1,
  // .session.last_will.retain = true,
};

static const char* TAG = "ESP::MQTT";


/* ==================== CRC32 Helper Functions ==================== */


/**
 * @brief Calculate CRC32 for given data
 * 
 * @param data Pointer to data buffer
 * @param length Length of data buffer
 * @return uint32_t Calculated CRC32 value
 */
static uint32_t mqttctrl_Crc32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFF;

  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
  }

  return crc ^ 0xFFFFFFFF;
}

/**
 * @brief Calculate CRC for mqtt_config_t structure (excluding crc field)
 * 
 * @param config Pointer to mqtt_config_t structure
 * @return uint32_t Calculated CRC32 value
 */
static uint32_t mqttctrl_ConfigCrc(const mqtt_config_t* config) {
  size_t crc_size = offsetof(mqtt_config_t, crc);
  return mqttctrl_Crc32((const uint8_t*)config, crc_size);
}

/**
 * @brief Validate mqtt_config_t structure integrity
 * 
 * @param config Pointer to mqtt_config_t structure
 * @return bool True if valid, false otherwise
 */
static bool mqttctrl_ValidateConfig(const mqtt_config_t* config) {
  uint32_t calculated_crc = mqttctrl_ConfigCrc(config);
  return (calculated_crc == config->crc);
}


/* ==================== NVS Configuration Partition Functions ==================== */

/**
 * @brief Read configuration from NVS config partition
 * 
 * @param key NVS key to read from
 * @param config_ptr Pointer to mqtt_config_t structure to store the read configuration
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_ConfigRead(const char* key, mqtt_config_t* config_ptr) {
  esp_err_t result = ESP_OK;
  size_t config_len = sizeof(mqtt_config_t);

  ESP_LOGI(TAG, "++%s(key: '%s', config_ptr: %p)", __func__, key, config_ptr);

  if (!mqtt_config_nvs_handle || !config_ptr) {
    ESP_LOGE(TAG, "[%s] Invalid parameters", __func__);
    return ESP_FAIL;
  }

  /* Read configuration from NVS */
  result = NVS_Read(mqtt_config_nvs_handle, key, config_ptr, &config_len);
  if (result != ESP_OK) {
    ESP_LOGD(TAG, "[%s] NVS_Read(key: '%s') not found or error", __func__, key);
    return result;
  }

  /* Validate CRC */
  if (mqttctrl_ValidateConfig(config_ptr)) {
    ESP_LOGD(TAG, "[%s] NVS_Read(key: '%s') OK - uri: '%s', port: %ld", __func__, 
          key, config_ptr->uri, config_ptr->port);
  } else {
    ESP_LOGW(TAG, "[%s] Config CRC invalid for key: '%s'", __func__, key);
    result = ESP_FAIL;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Write configuration to NVS config partition
 * 
 * @param key NVS key to write to
 * @param config_ptr Pointer to mqtt_config_t structure containing the configuration to write
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_ConfigWrite(const char* key, mqtt_config_t* config_ptr) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(key: '%s', config_ptr: %p)", __func__, key, config_ptr);

  if (!mqtt_config_nvs_handle || !config_ptr) {
    ESP_LOGE(TAG, "[%s] Invalid parameters", __func__);
    return ESP_FAIL;
  }

  /* Calculate and set CRC */
  config_ptr->crc = mqttctrl_ConfigCrc(config_ptr);

  /* Write configuration to NVS */
  result = NVS_Write(mqtt_config_nvs_handle, key, config_ptr, sizeof(mqtt_config_t));
  if (result == ESP_OK) {
    ESP_LOGD(TAG, "[%s] NVS_Write(key: '%s') OK - %zu bytes", __func__, 
          key, sizeof(mqtt_config_t));
  } else {
    ESP_LOGE(TAG, "[%s] NVS_Write(key: '%s') failed - result: %d", __func__, key, result);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Read active configuration slot from NVS
 * 
 * @param boot_slot Pointer to mqtt_slot_e variable to store the active slot
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_ConfigBootRead(mqtt_slot_e* boot_slot) {
  esp_err_t result = ESP_OK;
  size_t boot_size = sizeof(uint8_t);
  uint8_t boot_value = 0;

  ESP_LOGI(TAG, "++%s(boot_slot: %p)", __func__, boot_slot);

  if (!mqtt_config_nvs_handle || !boot_slot) {
    ESP_LOGE(TAG, "[%s] Invalid parameters", __func__);
    return ESP_FAIL;
  }

  result = NVS_Read(mqtt_config_nvs_handle, MQTT_CONFIG_BOOT_KEY, &boot_value, &boot_size);
  if (result == ESP_OK) {
    *boot_slot = (mqtt_slot_e)boot_value;
    ESP_LOGD(TAG, "[%s] Boot slot: %d", __func__, *boot_slot);
  } else {
    ESP_LOGD(TAG, "[%s] Boot slot not found, using default: %d", __func__, MQTT_SLOT_1);
    *boot_slot = MQTT_SLOT_1;
    result = ESP_OK;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Write active configuration slot to NVS
 * 
 * @param boot_slot Active configuration slot to write
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_ConfigBootWrite(mqtt_slot_e boot_slot) {
  esp_err_t result = ESP_OK;
  uint8_t boot_value = (uint8_t)boot_slot;

  ESP_LOGI(TAG, "++%s(boot_slot: %d)", __func__, boot_slot);
  
  if (!mqtt_config_nvs_handle) {
    ESP_LOGE(TAG, "[%s] Invalid parameters", __func__);
    return ESP_FAIL;
  }

  result = NVS_Write(mqtt_config_nvs_handle, MQTT_CONFIG_BOOT_KEY, &boot_value, sizeof(uint8_t));
  if (result == ESP_OK) {
    ESP_LOGD(TAG, "[%s] Boot slot written: %d", __func__, boot_slot);
  } else {
    ESP_LOGE(TAG, "[%s] NVS_Write failed - result: %d", __func__, result);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Initialize default configurations from build-time settings
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_InitDefaultConfigs(void) {
  esp_err_t result = ESP_OK;
  mqtt_config_t default_config;
  mqtt_config_t slot1_config, slot2_config;

  ESP_LOGI(TAG, "++%s()", __func__);

  /* Create default configuration from #defines */
  memset(&default_config, 0, sizeof(mqtt_config_t));
  default_config.fails = 0;
  strncpy(default_config.uri, CONFIG_BROKER_URL, MQTT_URI_SIZE - 1);
  default_config.uri[MQTT_URI_SIZE - 1] = '\0';
  default_config.port = CONFIG_BROKER_PORT;
  strncpy(default_config.username, CONFIG_CREDENTIAL_USERNAME, MQTT_USERNAME_SIZE - 1);
  default_config.username[MQTT_USERNAME_SIZE - 1] = '\0';
  strncpy(default_config.password, CONFIG_CREDENTIAL_PASSWORD, MQTT_PASSWORD_SIZE - 1);
  default_config.password[MQTT_PASSWORD_SIZE - 1] = '\0';

  ESP_LOGD(TAG, "[%s] Default config: uri='%s', port=%ld, user='%s'", __func__,
      default_config.uri, default_config.port, default_config.username);

  /* Check if slot 1 exists and is valid */
  memset(&slot1_config, 0, sizeof(mqtt_config_t));
  if (mqttctrl_ConfigRead(MQTT_CONFIG_SLOT1_KEY, &slot1_config) == ESP_OK) {
    ESP_LOGD(TAG, "[%s] Slot 1 is valid", __func__);
  } else {
    ESP_LOGD(TAG, "[%s] Slot 1 is invalid or missing, initializing with defaults", __func__);
    result = mqttctrl_ConfigWrite(MQTT_CONFIG_SLOT1_KEY, &default_config);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] Failed to write default config to slot 1", __func__);
      return result;
    }
  }

  /* Check if slot 2 exists and is valid */
  memset(&slot2_config, 0, sizeof(mqtt_config_t));
  if (mqttctrl_ConfigRead(MQTT_CONFIG_SLOT2_KEY, &slot2_config) == ESP_OK) {
    ESP_LOGD(TAG, "[%s] Slot 2 is valid", __func__);
  } else {
    ESP_LOGD(TAG, "[%s] Slot 2 is invalid or missing, initializing with defaults", __func__);
    result = mqttctrl_ConfigWrite(MQTT_CONFIG_SLOT2_KEY, &default_config);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] Failed to write default config to slot 2", __func__);
      return result;
    }
  }

  /* Set boot slot to slot 1 if not already set */
  mqtt_slot_e boot_slot = MQTT_SLOT_1;
  result = mqttctrl_ConfigBootRead(&boot_slot);
  if (result != ESP_OK || (boot_slot != MQTT_SLOT_1 && boot_slot != MQTT_SLOT_2)) {
    ESP_LOGD(TAG, "[%s] Boot slot invalid or missing, setting to slot 1", __func__);
    result = mqttctrl_ConfigBootWrite(MQTT_SLOT_1);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] Failed to write boot slot", __func__);
      return result;
    }
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Initialize configuration partition
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_InitConfigPartition(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  result = NVS_Open(MQTT_CONFIG_PARTITION, &mqtt_config_nvs_handle);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] NVS_Open('%s') failed - result: %d", __func__, MQTT_CONFIG_PARTITION, result);
    return result;
  }

  /* Initialize default configurations if missing or corrupted */
  result = mqttctrl_InitDefaultConfigs();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] Failed to initialize default configurations", __func__);
    return result;
  }

  /* Read boot slot to determine active configuration */
  result = mqttctrl_ConfigBootRead(&mqtt_slot);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] Failed to read boot slot", __func__);
    mqtt_slot = MQTT_SLOT_1;
  }

  mqtt_slot_e passive_slot = MQTT_GET_PASSIVE_SLOT(mqtt_slot);
  ESP_LOGD(TAG, "[%s] Active slot: %d, Passive slot: %d", __func__, mqtt_slot, passive_slot);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Close configuration partition
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_DoneConfigPartition(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  if (mqtt_config_nvs_handle) {
    result = NVS_Close(mqtt_config_nvs_handle);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] NVS_Close failed - result: %d", __func__, result);
    }
    mqtt_config_nvs_handle = NULL;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Get configuration slot key name
 * 
 * @param slot Configuration slot
 * @return const char* NVS key name for the given slot
 */
static const char* mqttctrl_GetSlotKey(mqtt_slot_e slot) {
  return (slot == MQTT_SLOT_1) ? MQTT_CONFIG_SLOT1_KEY : MQTT_CONFIG_SLOT2_KEY;
}

/**
 * @brief Validate MQTT URI format
 * Checks if URI starts with mqtt:// or mqtts://
 * 
 * @param uri Pointer to URI string
 * @return bool True if valid, false otherwise
 */
static bool mqttctrl_ValidateUri(const char* uri) {
  if (!uri || strlen(uri) < 7) {
    return false;
  }
  return (strncmp(uri, "mqtt://", 7) == 0) || (strncmp(uri, "mqtts://", 8) == 0);
}

/**
 * @brief Load active configuration from NVS and apply to mqtt_cfg
 * Copies configuration to static buffers to avoid dangling pointers
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_LoadActiveConfig(void) {
  esp_err_t result = ESP_OK;
  mqtt_config_t config;
  const char* slot_key = mqttctrl_GetSlotKey(mqtt_slot);

  ESP_LOGI(TAG, "++%s()", __func__);

  memset(&config, 0, sizeof(mqtt_config_t));

  result = mqttctrl_ConfigRead(slot_key, &config);
  if (result == ESP_OK) {
    ESP_LOGD(TAG, "[%s] Loaded config from slot %d: uri='%s', port=%ld", 
            __func__, mqtt_slot, config.uri, config.port);

    /* Copy configuration to static buffers */
    memset(mqtt_cfg_uri, 0, MQTT_URI_SIZE);
    strncpy(mqtt_cfg_uri, config.uri, MQTT_URI_SIZE - 1);
    mqtt_cfg_uri[MQTT_URI_SIZE - 1] = '\0';
    mqtt_cfg.broker.address.uri = mqtt_cfg_uri;

    memset(mqtt_cfg_username, 0, MQTT_USERNAME_SIZE);
    if (strlen(config.username) > 0) {
      strncpy(mqtt_cfg_username, config.username, MQTT_USERNAME_SIZE - 1);
      mqtt_cfg_username[MQTT_USERNAME_SIZE - 1] = '\0';
      mqtt_cfg.credentials.username = mqtt_cfg_username;
    }

    memset(mqtt_cfg_password, 0, MQTT_PASSWORD_SIZE);
    if (strlen(config.password) > 0) {
      strncpy(mqtt_cfg_password, config.password, MQTT_PASSWORD_SIZE - 1);
      mqtt_cfg_password[MQTT_PASSWORD_SIZE - 1] = '\0';
      mqtt_cfg.credentials.authentication.password = mqtt_cfg_password;
    }
  } else {
    ESP_LOGW(TAG, "[%s] Failed to load config from slot %d, using defaults", __func__, mqtt_slot);
    result = ESP_OK;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Handle successful connection confirmation after config update
 * Switches active to passive slot and updates boot slot in NVS
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_ConfirmConfigUpdate(void) {
  esp_err_t result = ESP_OK;
  mqtt_slot_e new_active = MQTT_GET_PASSIVE_SLOT(mqtt_slot);

  ESP_LOGI(TAG, "++%s()", __func__);

  /* Update boot slot in NVS with new active slot */
  result = mqttctrl_ConfigBootWrite(new_active);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] Failed to update boot slot", __func__);
  } else {
    mqtt_slot = new_active;
    ESP_LOGD(TAG, "[%s] Configuration update confirmed. Active slot now: %d", __func__, mqtt_slot);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief MQTT event handler
 *
 * @param handler_args Pointer to handler arguments
 * @param base Event base
 * @param event_id Event ID
 * @param event_data Event data
 */
static void mqttctrl_EventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = event_data;
  msg_t msg;
  bool send = false;

  ESP_LOGI(TAG, "++%s(handler_args: %p, base: %s, event_id: %ld, event_data: %p)", __func__, handler_args, base ? base : "-", event_id, event_data);
  ESP_LOGD(TAG, "[%s] event_id: %d [%s]", __func__, event->event_id, GET_MQTT_EVENT_NAME(event->event_id));

  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED: {
      /* If config update was in progress, confirm it on successful connection */
      if (mqtt_config_update_in_progress) {
        ESP_LOGD(TAG, "[%s] Connected with new config, confirming update", __func__);
        if (mqttctrl_ConfirmConfigUpdate() == ESP_OK) {
          mqtt_config_update_in_progress = false;
        } else {
          ESP_LOGE(TAG, "[%s] Failed to confirm config update", __func__);
        }
      }

      msg.type = MSG_TYPE_MQTT_EVENT;
      msg.from = REG_MQTT_CTRL;
      msg.to = REG_ALL_CTRL;
      msg.payload.mqtt.u.event_id = DATA_MQTT_EVENT_CONNECTED;
      send = true;
      break;
    }
    case MQTT_EVENT_DISCONNECTED: {
      msg.type = MSG_TYPE_MQTT_EVENT;
      msg.from = REG_MQTT_CTRL;
      msg.to = REG_ALL_CTRL;
      msg.payload.mqtt.u.event_id = DATA_MQTT_EVENT_DISCONNECTED;
      send = true;
      break;
    }
    case MQTT_EVENT_SUBSCRIBED: {
      break;
    }
    case MQTT_EVENT_UNSUBSCRIBED: {
      esp_mqtt_client_disconnect(mqtt_client);
      break;
    }
    case MQTT_EVENT_PUBLISHED: {
      break;
    }
    case MQTT_EVENT_DATA: {
      msg.type = MSG_TYPE_MQTT_DATA;
      msg.from = REG_MQTT_CTRL;
      msg.to = REG_MGR_CTRL;

      ESP_LOGD(TAG, " SIZE: topic: %d, data: %d", event->topic_len, event->data_len);

      if (event->topic_len && (event->topic_len < DATA_TOPIC_SIZE)) {
        memcpy(msg.payload.mqtt.u.data.topic, event->topic, event->topic_len);
        msg.payload.mqtt.u.data.topic[event->topic_len] = 0;
      } else {
        ESP_LOGE(TAG, "[%s] Wrong topic size: %d", __func__, event->topic_len);
        break;
      }

      if (event->data_len && (event->data_len < DATA_MSG_SIZE)) {
        memcpy(msg.payload.mqtt.u.data.msg, event->data, event->data_len);
        msg.payload.mqtt.u.data.msg[event->data_len] = 0;

        /* remove all unnecessary characters */
        cJSON_Minify(msg.payload.mqtt.u.data.msg);

      } else {
        ESP_LOGE(TAG, "[%s] Wrong data size: %d", __func__, event->data_len);
        break;
      }

      ESP_LOGD(TAG, "TOPIC: [%3d] '%s'", strlen(msg.payload.mqtt.u.data.topic), msg.payload.mqtt.u.data.topic);
      ESP_LOGD(TAG, " DATA: [%3d] '%s'", strlen(msg.payload.mqtt.u.data.msg), msg.payload.mqtt.u.data.msg);

      send = true;
      break;
    }
    case MQTT_EVENT_BEFORE_CONNECT: {
      break;
    }
    case MQTT_EVENT_ERROR: {
      ESP_LOGD(TAG, "[%s] MQTT_EVENT_ERROR", __func__);
      break;
    }
    default: {
      ESP_LOGD(TAG, "[%s] Unknown event_id: %d", __func__, event->event_id);
      break;
    }
  }
  if (send) {
    ESP_LOGD(TAG, "[%s] MGR_Send() -> msg.type: %d [%s]", __func__, 
        msg.type, GET_MSG_TYPE_NAME(msg.type));
    if (MGR_Send(&msg) != ESP_OK) {
      ESP_LOGE(TAG, "[%s] Message error. type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__,
          msg.type, GET_MSG_TYPE_NAME(msg.type),
          msg.from, msg.to);
    }
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Initialize MQTT client
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_InitClient(void) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);
  ESP_LOGD(TAG, "[%s] Broker: %s", __func__, mqtt_cfg.broker.address.uri);
  ESP_LOGD(TAG, "[%s]   '%s':'%s'", __func__, mqtt_cfg.credentials.username, mqtt_cfg.credentials.authentication.password);
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  if (mqtt_client) {
    result = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqttctrl_EventHandler, NULL);
  } else {
    ESP_LOGE(TAG, "[%s] esp_mqtt_client_init() failed", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Deinitialize MQTT client
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_DoneClient(void) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (mqtt_client) {
    result = esp_mqtt_client_unregister_event(mqtt_client, ESP_EVENT_ANY_ID, mqttctrl_EventHandler);
    ESP_LOGD(TAG, "[%s] esp_mqtt_client_unregister_event() - result: %d", __func__, result);
    result = esp_mqtt_client_stop(mqtt_client);
    ESP_LOGD(TAG, "[%s] esp_mqtt_client_stop() - result: %d", __func__, result);
    result = esp_mqtt_client_destroy(mqtt_client);
    ESP_LOGD(TAG, "[%s] esp_mqtt_client_destroy() - result: %d", __func__, result);
    mqtt_client = NULL;
  } else {
    ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Start MQTT client
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_StartClient(void) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (mqtt_client) {
    result = esp_mqtt_client_start(mqtt_client);
  } else {
    ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Stop MQTT client
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_StopClient(void) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (mqtt_client) {
    result = esp_mqtt_client_stop(mqtt_client);
  } else {
    ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Reconnect MQTT client with new configuration
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_ReconnectClient(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  result = mqttctrl_DoneClient();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] Failed to stop client", __func__);
    return result;
  }

  /* Small delay to ensure clean disconnect */
  vTaskDelay(pdMS_TO_TICKS(500));

  result = mqttctrl_InitClient();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] Failed to reinit client", __func__);
    return result;
  }

  result = mqttctrl_StartClient();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] Failed to start client", __func__);
    return result;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Parse and apply new MQTT configuration from JSON
 * Writes to pending slot and triggers reconnection
 * 
 * @param config_obj Pointer to cJSON object containing the configuration
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 * 
 * Expected JSON format (from README.md):
 * {
 *   "operation": "set",
 *   "broker": {
 *     "address": {
 *       "uri": "mqtt://broker.example.com",
 *       "port": 1883
 *     },
 *     "username": "user",
 *     "password": "pass"
 *   }
 * }
 */
static esp_err_t mqttctrl_SetConfig(const cJSON* config_obj) {
  esp_err_t result = ESP_OK;
  mqtt_config_t new_config;
  const char* pending_key;

  ESP_LOGI(TAG, "++%s(config_obj: %p)", __func__, config_obj);

  if (!config_obj) {
    ESP_LOGE(TAG, "[%s] Null config object", __func__);
    return ESP_FAIL;
  }

  memset(&new_config, 0, sizeof(mqtt_config_t));
  new_config.fails = 0;

  /* Navigate to broker.address object */
  cJSON* broker_obj = cJSON_GetObjectItem(config_obj, "broker");
  if (!cJSON_IsObject(broker_obj)) {
    ESP_LOGE(TAG, "[%s] Missing 'broker' object", __func__);
    return ESP_FAIL;
  }

  cJSON* address_obj = cJSON_GetObjectItem(broker_obj, "address");
  if (!cJSON_IsObject(address_obj)) {
    ESP_LOGE(TAG, "[%s] Missing 'broker.address' object", __func__);
    return ESP_FAIL;
  }

  /* Parse URI from broker.address.uri */
  cJSON* uri_obj = cJSON_GetObjectItem(address_obj, "uri");
  if (cJSON_IsString(uri_obj)) {
    const char* uri = cJSON_GetStringValue(uri_obj);

    /* Validate URI format */
    if (!mqttctrl_ValidateUri(uri)) {
      ESP_LOGE(TAG, "[%s] Invalid URI format (must start with mqtt:// or mqtts://): '%s'", __func__, uri);
      return ESP_FAIL;
    }

    strncpy(new_config.uri, uri, MQTT_URI_SIZE - 1);
    new_config.uri[MQTT_URI_SIZE - 1] = '\0';
    ESP_LOGD(TAG, "[%s] URI: '%s'", __func__, new_config.uri);
  } else {
    ESP_LOGE(TAG, "[%s] Missing or invalid 'broker.address.uri' field", __func__);
    return ESP_FAIL;
  }

  /* Parse Port from broker.address.port */
  cJSON* port_obj = cJSON_GetObjectItem(address_obj, "port");
  if (cJSON_IsNumber(port_obj)) {
    new_config.port = (uint32_t)cJSON_GetNumberValue(port_obj);
    if (new_config.port == 0 || new_config.port > 65535) {
      ESP_LOGE(TAG, "[%s] Invalid port: %ld", __func__, new_config.port);
      return ESP_FAIL;
    }
    ESP_LOGD(TAG, "[%s] Port: %ld", __func__, new_config.port);
  } else {
    /* Use default port if not specified */
    new_config.port = 1883;
    ESP_LOGD(TAG, "[%s] Port not specified, using default: 1883", __func__);
  }

  /* Parse Username (optional) - from broker.username if present */
  cJSON* username_obj = cJSON_GetObjectItem(broker_obj, "username");
  if (cJSON_IsString(username_obj)) {
    const char* username = cJSON_GetStringValue(username_obj);
    strncpy(new_config.username, username, MQTT_USERNAME_SIZE - 1);
    new_config.username[MQTT_USERNAME_SIZE - 1] = '\0';
    ESP_LOGD(TAG, "[%s] Username: '%s'", __func__, new_config.username);
  }

  /* Parse Password (optional) - from broker.password if present */
  cJSON* password_obj = cJSON_GetObjectItem(broker_obj, "password");
  if (cJSON_IsString(password_obj)) {
    const char* password = cJSON_GetStringValue(password_obj);
    strncpy(new_config.password, password, MQTT_PASSWORD_SIZE - 1);
    new_config.password[MQTT_PASSWORD_SIZE - 1] = '\0';
    ESP_LOGD(TAG, "[%s] Password: (hidden)", __func__);
  }

  /* Write to passive slot */
  mqtt_slot_e passive_slot = MQTT_GET_PASSIVE_SLOT(mqtt_slot);
  pending_key = mqttctrl_GetSlotKey(passive_slot);
  result = mqttctrl_ConfigWrite(pending_key, &new_config);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] Failed to write config to slot %d", __func__, passive_slot);
    return result;
  }

  /* Update mqtt_cfg for reconnection - copy to static buffers */
  memset(mqtt_cfg_uri, 0, MQTT_URI_SIZE);
  strncpy(mqtt_cfg_uri, new_config.uri, MQTT_URI_SIZE - 1);
  mqtt_cfg_uri[MQTT_URI_SIZE - 1] = '\0';
  mqtt_cfg.broker.address.uri = mqtt_cfg_uri;

  memset(mqtt_cfg_username, 0, MQTT_USERNAME_SIZE);
  if (strlen(new_config.username) > 0) {
    strncpy(mqtt_cfg_username, new_config.username, MQTT_USERNAME_SIZE - 1);
    mqtt_cfg_username[MQTT_USERNAME_SIZE - 1] = '\0';
    mqtt_cfg.credentials.username = mqtt_cfg_username;
  }

  memset(mqtt_cfg_password, 0, MQTT_PASSWORD_SIZE);
  if (strlen(new_config.password) > 0) {
    strncpy(mqtt_cfg_password, new_config.password, MQTT_PASSWORD_SIZE - 1);
    mqtt_cfg_password[MQTT_PASSWORD_SIZE - 1] = '\0';
    mqtt_cfg.credentials.authentication.password = mqtt_cfg_password;
  }

  /* Set flag indicating config update in progress */
  mqtt_config_update_in_progress = true;

  /* Trigger reconnection with new config */
  result = mqttctrl_ReconnectClient();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] Failed to reconnect with new config", __func__);
    mqtt_config_update_in_progress = false;
    return result;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Parse MQTT data JSON and execute operations
 * 
 * @param json_str JSON string to parse
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_ParseMqttData(const char* json_str) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(json_str: '%s')", __func__, json_str);
  cJSON* root = cJSON_Parse(json_str);
  if (root == NULL) {
    ESP_LOGE(TAG, "[%s] Unknown root: '%s'", __func__, json_str);
    return ESP_FAIL;
  }

  const cJSON* operation = cJSON_GetObjectItem(root, "operation");
  if (operation) {
    const char* o_str = cJSON_GetStringValue(operation);
    ESP_LOGD(TAG, "[%s] operation: '%s'", __func__, o_str);

    if (strcmp(o_str, "set") == 0) {
      result = mqttctrl_SetConfig(cJSON_GetObjectItem(root, "broker"));
    } else if (strcmp(o_str, "get") == 0) {
      ESP_LOGD(TAG, "[%s] GET operation not yet implemented", __func__);
      result = ESP_OK;
    } else {
      ESP_LOGW(TAG, "[%s] Unknown operation: '%s'", __func__, o_str);
    }
  } else {
    ESP_LOGE(TAG, "[%s] Bad data format. Missing operation field.", __func__);
    ESP_LOGE(TAG, "[%s] '%s'", __func__, cJSON_PrintUnformatted(root));
  }
  cJSON_Delete(root);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Publish message to MQTT topic
 * 
 * @param topic Pointer to topic string
 * @param msg Pointer to message string
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_Publish(const char* topic, const char* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(topic: '%s', msg: '%s')", __func__, topic, msg);
  int msg_id = esp_mqtt_client_publish(mqtt_client, topic, msg, 0, 1, 0);
  ESP_LOGD(TAG, "[%s] PUBLISH(topic: '%s', msg: '%s') -> msg_id: %d", __func__,
      topic, msg, msg_id);
  if (msg_id == -1) {
    result = ESP_FAIL;
  } else if (msg_id == -2) {
    result = ESP_ERR_NO_MEM;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Subscribe to MQTT topic
 * 
 * @param topic Pointer to topic string
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_SubscribeTopic(const char* topic) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(topic: '%s')", __func__, topic);
  int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, 0);
  ESP_LOGD(TAG, "[%s] SUBSCRIBE(topic: '%s') -> msg_id: %d", __func__, topic, msg_id);
  if (msg_id == -1) {
    result = ESP_FAIL;
  } else if (msg_id == -2) {
    result = ESP_ERR_NO_MEM;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Subscribe to multiple MQTT topics from JSON list
 * 
 * @param json_ptr Pointer to JSON string containing topic list
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 * 
 * Expected JSON format:
 * {
 *   "topics": [
 *     "topic/one",
 *     "topic/two",
 *     "topic/three"
 *   ]
 * }
 */
static esp_err_t mqttctrl_SubscribeList(const char* json_ptr) {
  cJSON *root;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(json_ptr: '%s')", __func__, json_ptr);
  root = cJSON_Parse(json_ptr);
  if (root != NULL)
  {
    cJSON* list = cJSON_GetObjectItem(root, "topics");
    for (int idx = 0; idx < cJSON_GetArraySize(list); ++idx) {
      char* topic = cJSON_GetStringValue(cJSON_GetArrayItem(list, idx));
      if (topic) {
        ESP_LOGD(TAG, "[%s] topic[idx=%d]: '%s'", __func__, idx, topic);
        int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, 0);
        ESP_LOGD(TAG, "[%s] SUBSCRIBE(topic: '%s') -> msg_id: %d", __func__, topic, msg_id);
        if (msg_id == -1) {
          result = ESP_FAIL;
        } else if (msg_id == -2) {
          result = ESP_ERR_NO_MEM;
        }
        if (result != ESP_OK) {
          break;
        }
      }
    }
    cJSON_Delete(root);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Parse incoming message and execute corresponding action
 * 
 * @param msg Pointer to message
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_ParseMsg(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(type: %d [%s], from: 0x%08lx, to: 0x%08lx)", __func__, 
      msg->type, GET_MSG_TYPE_NAME(msg->type),
      msg->from, msg->to);
  switch (msg->type) {
    case MSG_TYPE_INIT: {
      result = ESP_TASK_INIT;
      break;
    }
    case MSG_TYPE_DONE: {
      result = ESP_TASK_DONE;
      break;
    }
    case MSG_TYPE_RUN: {
      result = ESP_TASK_RUN;
      break;
    }

    case MSG_TYPE_MGR_UID: {
      memcpy(esp_uid, msg->payload.mgr.uid, strlen(msg->payload.mgr.uid) + 1);
      ESP_LOGD(TAG, "[%s] UID: '%s'", __func__, esp_uid);
      break;
    }

    case MSG_TYPE_MQTT_START: {
      result = mqttctrl_StartClient();
      if (result != ESP_OK) {
        ESP_LOGE(TAG, "[%s] mqttctrl_StartClient() - result: %d", __func__, result);
      }
      break;
    }
    case MSG_TYPE_MQTT_EVENT: {
      data_mqtt_event_e event_id = msg->payload.mqtt.u.event_id;

      ESP_LOGD(TAG, "[%s] event_id: %d [%s]", __func__, event_id, GET_DATA_MQTT_EVENT_NAME(event_id));
      if (event_id == DATA_MQTT_EVENT_DISCONNECTED) {
        result = mqttctrl_StopClient();
      } else if (event_id == DATA_MQTT_EVENT_CONNECTED) {
        /* Config update confirmation is handled in event handler */
      }
      break;
    }
    case MSG_TYPE_MQTT_DATA: {
      const data_mqtt_data_t* data_ptr = &(msg->payload.mqtt.u.data);

      ESP_LOGD(TAG, "[%s] topic: '%s'", __func__, data_ptr->topic);
      ESP_LOGD(TAG, "[%s]   msg: '%s'", __func__, data_ptr->msg);
      result = mqttctrl_ParseMqttData(data_ptr->msg);
      break;
    }
    case MSG_TYPE_MQTT_PUBLISH: {
      const data_mqtt_data_t* data_ptr = &(msg->payload.mqtt.u.data);

      result = mqttctrl_Publish(data_ptr->topic, data_ptr->msg);
      break;
    }
    case MSG_TYPE_MQTT_SUBSCRIBE: {
      result = mqttctrl_SubscribeTopic(msg->payload.mqtt.u.topic);
      break;
    }
    case MSG_TYPE_MQTT_SUBSCRIBE_LIST: {
      result = mqttctrl_SubscribeList(msg->payload.mqtt.u.json);
      break;
    }

    default: {
      ESP_LOGW(TAG, "[%s] Unknown message type: %d [%s]", __func__, msg->type, GET_MSG_TYPE_NAME(msg->type));
      result = ESP_FAIL;
      break;
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief MQTT control task function
 * 
 * @param param Task parameter (unused)
 */
static void mqttctrl_TaskFn(void* param) {
  msg_t msg;
  bool loop = true;
  esp_err_t result;

  ESP_LOGI(TAG, "++%s()", __func__);
  memset(&msg, 0x00, sizeof(msg_t));
  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    if(xQueueReceive(mqtt_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__, 
          msg.type, GET_MSG_TYPE_NAME(msg.type),
          msg.from, msg.to);

      result = mqttctrl_ParseMsg(&msg);
      if (result == ESP_TASK_DONE) {
        loop = false;
        result = ESP_OK;
      }

      if (result != ESP_OK) {
        // TODO - Send Error to the Broker
        ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
      }
    } else {
      ESP_LOGE(TAG, "[%s] Message error.", __func__);
    }
  }
  if (mqtt_sem_id) {
    xSemaphoreGive(mqtt_sem_id);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Send message to the MQTT controller task
 * 
 * @param msg Pointer to message
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(mqtt_msg_queue, msg, (TickType_t) 0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d, from: 0x%08lx, to: 0x%08lx", __func__, msg->type, msg->from, msg->to);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Initialize MQTT controller
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_Init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  /* Initialization message queue */
  mqtt_msg_queue = xQueueCreate(MQTT_MSG_MAX, sizeof(msg_t));
  if (mqtt_msg_queue == NULL)
  {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  mqtt_sem_id = xSemaphoreCreateCounting(1, 0);
  if (mqtt_sem_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xSemaphoreCreateCounting() failed.", __func__);
    return ESP_FAIL;
  }

  /* Get MAC address */
  if (tools_GetMacAddress(esp_mac, ESP_MAC_BASE) != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tools_GetMacAddress() failed.", __func__);
    return ESP_FAIL;
  }

  /* Initialize configuration partition */
  result = mqttctrl_InitConfigPartition();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] Failed to initialize config partition", __func__);
    return result;
  }

  /* Load active configuration */
  result = mqttctrl_LoadActiveConfig();
  if (result != ESP_OK) {
    ESP_LOGW(TAG, "[%s] Failed to load active config, using defaults", __func__);
  }

  /* Initialization MQTT thread */
  xTaskCreate(mqttctrl_TaskFn, MQTT_TASK_NAME, MQTT_TASK_STACK_SIZE, NULL, MQTT_TASK_PRIORITY, &mqtt_task_id);
  if (mqtt_task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }

  result = mqttctrl_InitClient();
  if (result != ESP_OK)
  {
    ESP_LOGE(TAG, "[%s] mqttctrl_InitClient() - result: %d", __func__, result);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done MQTT controller
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t mqttctrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mqttctrl_DoneClient();
  if (mqtt_sem_id) {
    msg_t msg = {
      .type = MSG_TYPE_DONE,
      .from = REG_MQTT_CTRL,
      .to = REG_MQTT_CTRL,
    };
    result = mqttctrl_Send(&msg);
  
    ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake to finish task...", __func__);
    xSemaphoreTake(mqtt_sem_id, portMAX_DELAY);

    vSemaphoreDelete(mqtt_sem_id);
    ESP_LOGD(TAG, "[%s] Semaphore deleted", __func__);

    ESP_LOGD(TAG, "[%s] Task stopped", __func__);
  }
  if (mqtt_msg_queue) {
    vQueueDelete(mqtt_msg_queue);
    ESP_LOGD(TAG, "[%s] Queue deleted", __func__);
  }

  result = mqttctrl_DoneConfigPartition();

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run MQTT controller
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error  
 */
static esp_err_t mqttctrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}


/* ==================== Public API ==================== */


/**
 * @brief Initialize MQTT controller
 * 
 * \return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
esp_err_t MqttCtrl_Init(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_MQTT_CTRL_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mqttctrl_Init();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done MQTT controller
 * 
 * \return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
esp_err_t MqttCtrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mqttctrl_Done();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run MQTT controller
 * 
 * \return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
esp_err_t MqttCtrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mqttctrl_Run();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Send message to the MQTT controller thread
 * 
 * \return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
esp_err_t MqttCtrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mqttctrl_Send(msg);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
