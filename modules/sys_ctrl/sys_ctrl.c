/**
 * @file sys_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief System Controller
 * @version 0.1
 * @date 2025-02-12
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_mac.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"

#include "sdkconfig.h"

#include "err.h"
#include "msg.h"
#include "mgr_ctrl.h"
#include "sys_ctrl.h"
#include "tools.h"

#include "cJSON.h"

#include "err.h"
#include "lut.h"
#include "sys_lut.h"


#define SYS_TASK_NAME           "sys-task"
#define SYS_TASK_STACK_SIZE     4096
#define SYS_TASK_PRIORITY       12

#define SYS_MSG_MAX             10


static const char* TAG = "ESP::SYS";


static QueueHandle_t      sys_msg_queue = NULL;
static TaskHandle_t       sys_task_id = NULL;
static SemaphoreHandle_t  sys_sem_id = NULL;


static data_eth_mac_t     sys_esp_mac = {0};
static data_uid_t         esp_uid = {0};

#define SYS_NTP_DEFAULT_SERVER    CONFIG_SYS_CTRL_NTP_SERVER_DEFAULT
#define SYS_TIME_STR_SIZE         (32U)
#define SYS_NTP_SERVER_LEN        (64U)

static char   sys_ntp_servers[CONFIG_LWIP_SNTP_MAX_SERVERS][SYS_NTP_SERVER_LEN] = {};
static size_t sys_ntp_servers_count = 0;

typedef enum {
  SYS_FIELDS_TIMEZONE = (1U << 0),
  SYS_FIELDS_TIME     = (1U << 1),
  SYS_FIELDS_NTP      = (1U << 2),
  SYS_FIELDS_ALL      = (SYS_FIELDS_TIMEZONE | SYS_FIELDS_TIME | SYS_FIELDS_NTP),
} sys_fields_mask_e;


/**
 * @brief Time synchronization notification callback
 * 
 * Called when SNTP successfully synchronizes the time.
 *
 * @param tv Pointer to a timeval structure containing the synchronized time
 */
static void sysctrl_TimeSyncNotificationCb(struct timeval *tv)
{
  ESP_LOGI(TAG, "[%s] Notification of a time synchronization event", __func__);
}

/**
 * @brief Initialize default NTP server list
 *
 * Sets up the default NTP server (from CONFIG_SYS_CTRL_NTP_SERVER_DEFAULT)
 * if no servers have been configured yet.
 */
static void sysctrl_InitDefaultNtpServers(void) {
  if (sys_ntp_servers_count == 0) {
    strncpy(sys_ntp_servers[0], SYS_NTP_DEFAULT_SERVER, SYS_NTP_SERVER_LEN - 1);
    sys_ntp_servers[0][SYS_NTP_SERVER_LEN - 1] = '\0';
    sys_ntp_servers_count = 1;
    
    ESP_LOGI(TAG, "[%s] Initialized default NTP server: %s",
             __func__, sys_ntp_servers[0]);
  }
}

/**
 * @brief Apply current NTP server list to SNTP configuration
 *
 * Deinitializes current SNTP, then reinitializes with new server list.
 * Resets active server index to 0 since we're starting fresh with new config.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_ApplyNtpServers(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  esp_netif_sntp_deinit();

  esp_sntp_config_t config = {
    .smooth_sync = false,
    .server_from_dhcp = false,
    .wait_for_sync = true,
    .start = true,
    .sync_cb = sysctrl_TimeSyncNotificationCb,
    .renew_servers_after_new_IP = false,
    .ip_event_to_renew = IP_EVENT_STA_GOT_IP,
    .index_of_first_server = 0,
    .num_of_servers = sys_ntp_servers_count,
  };

  for (size_t i = 0; i < sys_ntp_servers_count; ++i) {
    config.servers[i] = sys_ntp_servers[i];
  }

  if (sys_ntp_servers_count > 0) {
    ESP_LOGI(TAG, "[%s] Applying NTP configuration with %zu servers",
             __func__, sys_ntp_servers_count);
  }

  result = esp_netif_sntp_init(&config);
  
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Initialize SNTP
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_InitSNTP(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  sysctrl_InitDefaultNtpServers();
  result = sysctrl_ApplyNtpServers();

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Wait for time to be set via SNTP
 * 
 */
static void sysctrl_WaitTime(void) {
  // wait for time to be set
  int retry = 0;
  const int retry_count = 15;

  ESP_LOGI(TAG, "++%s()", __func__);

  while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
    ESP_LOGD(TAG, "[%s] Waiting for system time to be set... (%d/%d)", __func__, retry, retry_count);
  }

  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Set time zone
 * 
 * @param tz_str Time zone string (e.g., "PST8PDT")
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_setTimeZone(const char* tz_str) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (setenv("TZ", tz_str, 1) == 0) {
    time_t now;
    struct tm tm;
    char buf[64];

    tzset();

    ESP_LOGD(TAG, "tzname[0]=%s tzname[1]=%s",
             tzname[0] ? tzname[0] : "-", tzname[1] ? tzname[1] : "-");

    now = time(NULL);
    localtime_r(&now, &tm);

    strftime(buf, sizeof(buf), "%Z %z", &tm);
    ESP_LOGD(TAG, "Current local zone: %s", buf);

  } else {
    ESP_LOGE(TAG, "setenv(TZ) failed");
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Get time zone information
 * 
 */
static void sysctrl_GetTimeZone(void) {
  const char *env_tz = getenv("TZ");

  ESP_LOGI(TAG, "++%s()", __func__);
  if (env_tz) {
    ESP_LOGD(TAG, "TZ env: %s", env_tz);
  } else {
    ESP_LOGW(TAG, "TZ env not set");
  }

  ESP_LOGD(TAG, "tzname[0]=%s tzname[1]=%s",
            tzname[0] ? tzname[0] : "-", tzname[1] ? tzname[1] : "-");

  time_t now = time(NULL);
  struct tm tm;
  localtime_r(&now, &tm);
  char buf[64];
  strftime(buf, sizeof(buf), "%Z %z", &tm); // %Z = zone name, %z = offset (+/-HHMM)
  ESP_LOGD(TAG, "Current local zone: %s", buf);

  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Get current time
 * 
 */
static void sysctrl_GetTime(void) {
  time_t now;
  char strftime_buf[64];
  struct tm timeinfo;

  ESP_LOGI(TAG, "++%s()", __func__);
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGD(TAG, "[%s] The current date/time in is: %s", __func__, strftime_buf);
  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Build time information JSON object
 *
 * Adds unix, local and UTC time representations to the provided JSON object.
 *
 * @param time_obj cJSON object to fill with time data
 */
static void sysctrl_BuildTimeInfo(cJSON* time_obj) {
  time_t now = 0;
  struct tm local_tm;
  struct tm utc_tm;
  char local_str[SYS_TIME_STR_SIZE] = {};
  char utc_str[SYS_TIME_STR_SIZE] = {};

  if (!time_obj) {
    return;
  }

  time(&now);
  localtime_r(&now, &local_tm);
  gmtime_r(&now, &utc_tm);

  strftime(local_str, sizeof(local_str), "%Y-%m-%d %H:%M:%S", &local_tm);
  strftime(utc_str, sizeof(utc_str), "%Y-%m-%d %H:%M:%S", &utc_tm);

  cJSON_AddNumberToObject(time_obj, "unix", (double) now);
  cJSON_AddStringToObject(time_obj, "local", local_str);
  cJSON_AddStringToObject(time_obj, "utc", utc_str);
}

/**
 * @brief Build NTP information JSON object
 *
 * Adds list of configured NTP servers and sync status.
 *
 * @param ntp_obj cJSON object to fill with NTP data
 */
static void sysctrl_BuildNtpInfo(cJSON* ntp_obj) {
  if (!ntp_obj) {
    return;
  }

  cJSON* servers = cJSON_AddArrayToObject(ntp_obj, "servers");

  if (sys_ntp_servers_count == 0) {
    sysctrl_InitDefaultNtpServers();
  }

  for (size_t idx = 0; idx < sys_ntp_servers_count; ++idx) {
    cJSON_AddItemToArray(servers, cJSON_CreateString(sys_ntp_servers[idx]));
  }

  sntp_sync_status_t status = sntp_get_sync_status();
  bool synced = (status == SNTP_SYNC_STATUS_COMPLETED) || (status == SNTP_SYNC_STATUS_IN_PROGRESS);
  cJSON_AddBoolToObject(ntp_obj, "synced", synced);
}

/**
 * @brief Parse requested fields list into a bitmask
 *
 * If fields is missing or not an array, returns SYS_FIELDS_ALL.
 *
 * @param fields cJSON array with field names
 * @return Bitmask of requested fields
 */
static sys_fields_mask_e sysctrl_ParseFields(const cJSON* fields) {
  if (!cJSON_IsArray(fields)) {
    return SYS_FIELDS_ALL;
  }

  sys_fields_mask_e mask = 0;
  const cJSON* field = NULL;
  cJSON_ArrayForEach(field, fields) {
    if (!cJSON_IsString(field) || (field->valuestring == NULL)) {
      continue;
    }
    if (strcmp(field->valuestring, "timezone") == 0) {
      mask |= SYS_FIELDS_TIMEZONE;
    } else if (strcmp(field->valuestring, "time") == 0) {
      mask |= SYS_FIELDS_TIME;
    } else if (strcmp(field->valuestring, "ntp") == 0) {
      mask |= SYS_FIELDS_NTP;
    }
  }

  return (mask == 0) ? SYS_FIELDS_ALL : mask;
}

/**
 * @brief Prepare and send MQTT response for SYS request
 *
 * Builds JSON response based on requested fields mask and publishes it to MQTT.
 *
 * @param fields_mask Bitmask of requested fields
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_PrepareResponseMask(sys_fields_mask_e fields_mask) {
  msg_t msg = {
    .type = MSG_TYPE_MQTT_PUBLISH,
    .from = REG_SYS_CTRL,
    .to = REG_MQTT_CTRL,
  };
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);

  cJSON* response = cJSON_CreateObject();
  if (response == NULL) {
    return ESP_FAIL;
  }

  cJSON_AddStringToObject(response, "operation", "response");

  if (fields_mask & SYS_FIELDS_TIMEZONE) {
    const char* tz = getenv("TZ");
    cJSON_AddStringToObject(response, "timezone", tz ? tz : "");
  }

  if (fields_mask & SYS_FIELDS_TIME) {
    cJSON* time_obj = cJSON_AddObjectToObject(response, "time");
    sysctrl_BuildTimeInfo(time_obj);
  }

  if (fields_mask & SYS_FIELDS_NTP) {
    cJSON* ntp_obj = cJSON_AddObjectToObject(response, "ntp");
    sysctrl_BuildNtpInfo(ntp_obj);
  }

  int ret = cJSON_PrintPreallocated(response, msg.payload.mqtt.u.data.msg, DATA_JSON_SIZE, 0);
  if (ret == 1) {
    snprintf(msg.payload.mqtt.u.data.topic, DATA_TOPIC_SIZE, "%s/res/sys", esp_uid);
    result = MGR_Send(&msg);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] MGR_Send() - Error: %d", __func__, result);
    }
  } else {
    ESP_LOGE(TAG, "[%s] cJSON_PrintPreallocated() - Error: %d", __func__, ret);
  }

  cJSON_Delete(response);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Prepare and send MQTT event for SYS set request
 *
 * Builds JSON event based on requested fields mask and publishes it to MQTT.
 *
 * @param fields_mask Bitmask of requested fields
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_PrepareEventMask(sys_fields_mask_e fields_mask) {
  msg_t msg = {
    .type = MSG_TYPE_MQTT_PUBLISH,
    .from = REG_SYS_CTRL,
    .to = REG_MQTT_CTRL,
  };
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);

  cJSON* event = cJSON_CreateObject();
  if (event == NULL) {
    return ESP_FAIL;
  }

  cJSON_AddStringToObject(event, "operation", "event");

  if (fields_mask & SYS_FIELDS_TIMEZONE) {
    const char* tz = getenv("TZ");
    cJSON_AddStringToObject(event, "timezone", tz ? tz : "");
  }

  if (fields_mask & SYS_FIELDS_TIME) {
    cJSON* time_obj = cJSON_AddObjectToObject(event, "time");
    sysctrl_BuildTimeInfo(time_obj);
  }

  if (fields_mask & SYS_FIELDS_NTP) {
    cJSON* ntp_obj = cJSON_AddObjectToObject(event, "ntp");
    sysctrl_BuildNtpInfo(ntp_obj);
  }

  int ret = cJSON_PrintPreallocated(event, msg.payload.mqtt.u.data.msg, DATA_JSON_SIZE, 0);
  if (ret == 1) {
    snprintf(msg.payload.mqtt.u.data.topic, DATA_TOPIC_SIZE, "%s/event/sys", esp_uid);
    result = MGR_Send(&msg);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] MGR_Send() - Error: %d", __func__, result);
    }
  } else {
    ESP_LOGE(TAG, "[%s] cJSON_PrintPreallocated() - Error: %d", __func__, ret);
  }

  cJSON_Delete(event);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Prepare and send MQTT response for SYS get request
 *
 * Builds JSON response based on requested fields and publishes it to MQTT.
 *
 * @param fields cJSON array with requested fields
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_PrepareResponse(const cJSON* fields) {
  return sysctrl_PrepareResponseMask(sysctrl_ParseFields(fields));
}

/**
 * @brief Parse time string in "YYYY-MM-DD HH:MM:SS" format
 *
 * @param time_str time string
 * @param tm_out output tm structure
 * @return true if parsing succeeded
 */
static bool sysctrl_ParseTimeString(const char* time_str, struct tm* tm_out) {
  if (!time_str || !tm_out) {
    return false;
  }

  memset(tm_out, 0, sizeof(*tm_out));
  return (strptime(time_str, "%Y-%m-%d %H:%M:%S", tm_out) != NULL);
}

/**
 * @brief Convert tm to time_t in UTC using temporary TZ override
 *
 * Since timegm() may not be available in all environments (including ESP-IDF),
 * we use a portable approach: temporarily set TZ to UTC, use mktime(), then restore.
 *
 * @param tm_utc time in UTC
 * @return time_t converted value
 */
static time_t sysctrl_TimegmFallback(struct tm* tm_utc) {
  const char* old_tz = getenv("TZ");
  char old_tz_buf[SYS_TIME_STR_SIZE] = {};
  bool had_tz = false;

  if (old_tz) {
    strncpy(old_tz_buf, old_tz, sizeof(old_tz_buf) - 1);
    old_tz_buf[sizeof(old_tz_buf) - 1] = '\0';
    had_tz = true;
  }

  setenv("TZ", "UTC0", 1);
  tzset();
  time_t t = mktime(tm_utc);

  if (had_tz) {
    setenv("TZ", old_tz_buf, 1);
  } else {
    unsetenv("TZ");
  }
  tzset();

  return t;
}

/**
 * @brief Set system time using unix timestamp
 *
 * @param unix_time unix timestamp
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_SetTimeUnix(time_t unix_time) {
  struct timeval tv = {
    .tv_sec = unix_time,
    .tv_usec = 0,
  };

  if (settimeofday(&tv, NULL) != 0) {
    return ESP_FAIL;
  }

  sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
  return ESP_OK;
}

/**
 * @brief Apply NTP server list from JSON array
 *
 * Parses server list from JSON, stores it in sys_ntp_servers[], and applies
 * the configuration. The active server index is reset to 0.
 *
 * @param servers JSON array with server strings
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_SetNtpServers(const cJSON* servers) {
  if (!cJSON_IsArray(servers)) {
    return ESP_ERR_INVALID_ARG;
  }

  size_t count = 0;
  const cJSON* item = NULL;
  cJSON_ArrayForEach(item, servers) {
    if (!cJSON_IsString(item) || (item->valuestring == NULL)) {
      continue;
    }
    if (count >= CONFIG_LWIP_SNTP_MAX_SERVERS) {
      break;
    }

    strncpy(sys_ntp_servers[count], item->valuestring, SYS_NTP_SERVER_LEN - 1);
    sys_ntp_servers[count][SYS_NTP_SERVER_LEN - 1] = '\0';
    ++count;
  }

  if (count == 0) {
    ESP_LOGE(TAG, "[%s] No valid NTP servers provided", __func__);
    return ESP_ERR_INVALID_ARG;
  }

  sys_ntp_servers_count = count;
  
  ESP_LOGI(TAG, "[%s] Configured %zu NTP servers", 
           __func__, sys_ntp_servers_count);

  return sysctrl_ApplyNtpServers();
}

/**
 * @brief Parse and apply SYS set request
 *
 * @param root JSON root object
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_ParseSet(const cJSON* root) {
  esp_err_t result = ESP_OK;
  sys_fields_mask_e fields_mask = 0;

  const cJSON* tz_obj = cJSON_GetObjectItem(root, "timezone");
  if (cJSON_IsString(tz_obj) && tz_obj->valuestring) {
    result = sysctrl_setTimeZone(tz_obj->valuestring);
    if (result == ESP_OK) {
      fields_mask |= SYS_FIELDS_TIMEZONE;
    }
  }

  const cJSON* time_obj = cJSON_GetObjectItem(root, "time");
  if (cJSON_IsObject(time_obj)) {
    const cJSON* unix_obj = cJSON_GetObjectItem(time_obj, "unix");
    const cJSON* local_obj = cJSON_GetObjectItem(time_obj, "local");
    const cJSON* utc_obj = cJSON_GetObjectItem(time_obj, "utc");

    if (cJSON_IsNumber(unix_obj)) {
      result = sysctrl_SetTimeUnix((time_t) unix_obj->valuedouble);
      if (result == ESP_OK) {
        fields_mask |= SYS_FIELDS_TIME;
      }
    } else if (cJSON_IsString(local_obj) && local_obj->valuestring) {
      struct tm tm_local;
      if (sysctrl_ParseTimeString(local_obj->valuestring, &tm_local)) {
        result = sysctrl_SetTimeUnix(mktime(&tm_local));
        if (result == ESP_OK) {
          fields_mask |= SYS_FIELDS_TIME;
        }
      } else {
        result = ESP_ERR_INVALID_ARG;
      }
    } else if (cJSON_IsString(utc_obj) && utc_obj->valuestring) {
      struct tm tm_utc;
      if (sysctrl_ParseTimeString(utc_obj->valuestring, &tm_utc)) {
        result = sysctrl_SetTimeUnix(sysctrl_TimegmFallback(&tm_utc));
        if (result == ESP_OK) {
          fields_mask |= SYS_FIELDS_TIME;
        }
      } else {
        result = ESP_ERR_INVALID_ARG;
      }
    }
  }

  const cJSON* ntp_obj = cJSON_GetObjectItem(root, "ntp");
  if (cJSON_IsObject(ntp_obj)) {
    const cJSON* servers = cJSON_GetObjectItem(ntp_obj, "servers");
    if (servers) {
      result = sysctrl_SetNtpServers(servers);
      if (result == ESP_OK) {
        fields_mask |= SYS_FIELDS_NTP;
      }
    }
  }

  if (fields_mask != 0) {
    result = sysctrl_PrepareResponseMask(fields_mask);
    if (result == ESP_OK) {
      sysctrl_PrepareEventMask(fields_mask);
    }
  }

  return result;
}

/**
 * @brief Parse MQTT JSON payload for SYS module
 *
 * Currently supports operation "get" and "set".
 *
 * @param json_str JSON payload string
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_ParseMqttData(const char* json_str) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(json_str: '%s')", __func__, json_str);
  cJSON* root = cJSON_Parse(json_str);
  if (root) {
    const cJSON* operation = cJSON_GetObjectItem(root, "operation");
    if (operation) {
      const char* o_str = cJSON_GetStringValue(operation);
      ESP_LOGD(TAG, "[%s] operation: '%s'", __func__, o_str);
      if (strcmp(o_str, "get") == 0) {
        const cJSON* fields = cJSON_GetObjectItem(root, "fields");
        result = sysctrl_PrepareResponse(fields);
      } else if (strcmp(o_str, "set") == 0) {
        result = sysctrl_ParseSet(root);
      } else {
        ESP_LOGW(TAG, "[%s] Unsupported operation: '%s'", __func__, o_str);
      }
    } else {
      ESP_LOGE(TAG, "[%s] Bad data format. Missing operation field.", __func__);
      ESP_LOGE(TAG, "[%s] '%s'", __func__, cJSON_PrintUnformatted(root));
    }
    cJSON_Delete(root);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Handle Ethernet events
 * 
 * @param event_id The specific event ID within the event base
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_EthNotify(data_eth_event_e event_id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(event_id: %d [%s])", __func__,
      event_id, GET_DATA_ETH_EVENT_NAME(event_id));

  switch (event_id) {
    case DATA_ETH_EVENT_CONNECTED: {
      ESP_LOGD(TAG, "Ethernet Link Up");
      sysctrl_InitSNTP();
      sysctrl_WaitTime();
      break;
    }
    case DATA_ETH_EVENT_DISCONNECTED: {
      ESP_LOGD(TAG, "Ethernet Link Down");
      break;
    }
    case DATA_ETH_EVENT_START: {
      ESP_LOGD(TAG, "Ethernet Started");
      break;
    }
    case DATA_ETH_EVENT_STOP: {
      ESP_LOGD(TAG, "Ethernet Stopped");
      break;
    }
    default: {
      break;
    }
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Parse incoming messages
 * 
 * @param msg Pointer to the message to be parsed
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_ParseMsg(const msg_t* msg) {
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

    case MSG_TYPE_ETH_EVENT: {
      sysctrl_EthNotify(msg->payload.eth.u.event_id);
      break;
    }

    case MSG_TYPE_MQTT_EVENT: {
      data_mqtt_event_e event_id = msg->payload.mqtt.u.event_id;
      ESP_LOGD(TAG, "[%s] event_id: %d [%s]", __func__, event_id, GET_DATA_MQTT_EVENT_NAME(event_id));
      break;
    }

    case MSG_TYPE_MQTT_DATA: {
      const data_mqtt_data_t* data_ptr = &(msg->payload.mqtt.u.data);

      ESP_LOGD(TAG, "[%s] topic: '%s'", __func__, data_ptr->topic);
      ESP_LOGD(TAG, "[%s]   msg: '%s'", __func__, data_ptr->msg);
      result = sysctrl_ParseMqttData(data_ptr->msg);
      break;
    }

    default: {
      result = ESP_FAIL;
      break;
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief System task's function
 * 
 * @param param 
 */
static void sysctrl_TaskFn(void* param) {
  msg_t msg;
  bool loop = true;
  esp_err_t result;

  ESP_LOGI(TAG, "++%s()", __func__);
  memset(&msg, 0x00, sizeof(msg_t));
  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    sysctrl_GetTime();
    if(xQueueReceive(sys_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__, 
          msg.type, GET_MSG_TYPE_NAME(msg.type),
          msg.from, msg.to);
      
      result = sysctrl_ParseMsg(&msg);
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
  if (sys_sem_id) {
    xSemaphoreGive(sys_sem_id);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Send message to System controller
 * 
 * @param msg Pointer to the message to be sent
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(sys_msg_queue, msg, (TickType_t) 0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d, from: 0x%08lx, to: 0x%08lx", __func__, msg->type, msg->from, msg->to);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Init System controller
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_Init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  sysctrl_GetTimeZone();

  /* Set timezone from configuration
    This is a TZ string in POSIX format that sets the time zone and DST rules.

    The function sysctrl_setTimeZone applies this with setenv("TZ", ...) and tzset(), so local time functions
    (localtime, strftime) will use those rules.
  */
#ifdef CONFIG_SYS_CTRL_TIMEZONE_CUSTOM
  const char* tz_string = CONFIG_SYS_CTRL_TIMEZONE_CUSTOM_STRING;
  if (!tz_string || tz_string[0] == '\0') {
    ESP_LOGW(TAG, "[%s] Custom timezone selected but string is empty, using UTC", __func__);
    tz_string = "UTC0";
  }
#else
  const char* tz_string = CONFIG_SYS_CTRL_TIMEZONE_STRING;
#endif
  sysctrl_setTimeZone(tz_string);

  /* Initialization message queue */
  sys_msg_queue = xQueueCreate(SYS_MSG_MAX, sizeof(msg_t));
  if (sys_msg_queue == NULL)
  {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  sys_sem_id = xSemaphoreCreateCounting(1, 0);
  if (sys_sem_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xSemaphoreCreateCounting() failed.", __func__);
    return ESP_FAIL;
  }

  /* Initialization thread */
  xTaskCreate(sysctrl_TaskFn, SYS_TASK_NAME, SYS_TASK_STACK_SIZE, NULL, SYS_TASK_PRIORITY, &sys_task_id);
  if (sys_task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }

  /* Get ESP_MAC_BASE and keep it in sys_esp_mac */
  /* The default base MAC is pre-programmed by Espressif in eFuse BLK0. */
  if (tools_GetMacAddress(sys_esp_mac, ESP_MAC_BASE) != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tools_GetMacAddress() failed.", __func__);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done System controller
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (sys_sem_id) {
    msg_t msg = {
      .type = MSG_TYPE_DONE,
      .from = REG_SYS_CTRL,
      .to = REG_SYS_CTRL,
    };
    result = sysctrl_Send(&msg);

    ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake to finish task...", __func__);
    xSemaphoreTake(sys_sem_id, portMAX_DELAY);

    vSemaphoreDelete(sys_sem_id);
    ESP_LOGD(TAG, "[%s] Semaphore deleted", __func__);

    ESP_LOGD(TAG, "[%s] Task stopped", __func__);
  }
  if (sys_msg_queue) {
    vQueueDelete(sys_msg_queue);
    ESP_LOGD(TAG, "[%s] Queue deleted", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run System controller
 * 
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Init System controller
 * 
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t SysCtrl_Init(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_SYS_CTRL_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  result = sysctrl_Init();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done System controller
 * 
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t SysCtrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = sysctrl_Done();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run System controller
 * 
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t SysCtrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = sysctrl_Run();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Send message to the System controller thread
 * 
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t SysCtrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = sysctrl_Send(msg);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
