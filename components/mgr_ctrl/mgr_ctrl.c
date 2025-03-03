/**
 * @file mgr_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief Manager Controller
 * @version 0.1
 * @date 2025-01-28
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cJSON.h"

#include "sdkconfig.h"

#include "mgr_ctrl.h"
#include "mgr_reg.h"

#include "mgr_reg_list.h"

#include "err.h"
#include "lut.h"


#define MGR_TASK_NAME           "mgr-task"
#define MGR_TASK_STACK_SIZE     4096
#define MGR_TASK_PRIORITY       8

#define MGR_MSG_MAX             40

#define GET_ETH_MAC(_mac)       _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]

#define MGR_TOPIC_MAX_LEN       (20U)
#define MGR_UID_LEN             (10U)
#define MGR_MAC_LEN             (17U)
#define MGR_IP_LEN              (15U)

#define MGR_TOPIC_LEN           (MGR_TOPIC_MAX_LEN - MGR_UID_LEN - 1)

#define MGR_UID_IDX             (0U)
#define MGR_TOPIC_IDX           (10U)


static const char* TAG = "ESP::MGR";


static int mgr_modules_cnt = MGR_REG_LIST_CNT;

static QueueHandle_t      mgr_msg_queue = NULL;
static TaskHandle_t       mgr_task_id = NULL;
static SemaphoreHandle_t  mgr_sem_id = NULL;

static data_eth_mac_t     mgr_eth_mac = {};
static data_eth_info_t    mgr_eth_info = {};


/**
 * @brief Buffer to prepare topic
 *
 * The buffer is used to prepare the topic.
 * It consists of the following parts:
 *
 *   UID/topic
 *      where:
 *        - UID - has 10 bytes: 'ESP_12AB34'
 *        - topic - has MQTT_TOPIC_LEN bytes: '/sys'
 */
//static char     mgr_topic_buffer[MGR_TOPIC_MAX_LEN];

static char mgr_reg_pattern[]   = "REGISTER/ESP";
static char mgr_uid_pattern[]   = "ESP_%02X%02X%02X";
static char mgr_mac_pattern[]   = "%02X:%02X:%02X:%02X:%02X:%02X";
static char mgr_ip_pattern[]    = "%d.%d.%d.%d";

static char mgr_topic_pattern[] = "%s/%s";

static char mgr_uid[MGR_UID_LEN + 1]  = {}; /* keeps only UID, as: ESP_12AB34 */
static char mgr_mac[MGR_MAC_LEN + 1]  = {}; /* keeps only MAC, as: 12:34:56:78:90:AB */
static char mgr_ip[MGR_IP_LEN + 1]    = {}; /* keeps only IP, as: 123.123.123.123 */


/**
 * @brief The variable holds a pointer to the send_fn() function to invoke it faster.
 * 
 */
static mgr_reg_send_f mgr_send_to_mqtt_fn = NULL;

typedef struct {
  uint32_t  type;
  char      topic[MGR_TOPIC_MAX_LEN];
} mgr_topic_t;

/**
 * @brief The variable holds the list of modules types along with its topic
 * 
 */
static mgr_topic_t  mgr_topic_list[MGR_REG_LIST_CNT] = {};


static esp_err_t mgr_Init(int id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(id: %d, type: 0x%08lx)", __func__, id, mgr_reg_list[id].type);
  if (mgr_reg_list[id].init_fn) {
    result = mgr_reg_list[id].init_fn();
    if (mgr_reg_list[id].type & REG_MQTT_CTRL) {
      mgr_send_to_mqtt_fn = mgr_reg_list[id].send_fn;
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_Done(int id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(id: %d, type: 0x%08lx)", __func__, id, mgr_reg_list[id].type);
  if (mgr_reg_list[id].done_fn) {
    result = mgr_reg_list[id].done_fn();
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_Run(int id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(id: %d, type: 0x%08lx)", __func__, id, mgr_reg_list[id].type);
  if (mgr_reg_list[id].run_fn) {
    result = mgr_reg_list[id].run_fn();
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(mgr_msg_queue, msg, (TickType_t) 0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__, 
        msg->type, GET_MSG_TYPE_NAME(msg->type),
        msg->from, msg->to);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Create a UID
 *
 * Creates UID from MAC address.
 * It will be as: ESP_12AB34
 *
 */
static void mgr_CreateUid(void) {
  ESP_LOGI(TAG, "++%s()", __func__);

  sprintf(mgr_uid, mgr_uid_pattern, mgr_eth_mac[3], mgr_eth_mac[4], mgr_eth_mac[5]);
  ESP_LOGD(TAG, "[%s]     mgr_uid: '%s'", __func__, mgr_uid);

  sprintf(mgr_mac, mgr_mac_pattern, mgr_eth_mac[0], mgr_eth_mac[1], mgr_eth_mac[2], mgr_eth_mac[3], mgr_eth_mac[4], mgr_eth_mac[5]);
  ESP_LOGD(TAG, "[%s]     mgr_mac: '%s'", __func__, mgr_mac);

  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Create a list of registered modules
 *
 * data.topic: 'REGISTER/ESP'
 * data.msg: JSON format
 *  {
 *    "operation": "event",
 *    "uid": "ESP_12AB34",
 *    "mac": "12:34:56:78:90:AB",
 *    "ip": "xxx.xxx.xxx.xxx",
 *     "list": ["eth", "mqtt"]
 *   }
 *
 */
void mgr_CreateModuleList(void) {
  const char json_str[] = "{\"operation\": \"event\",\"uid\": \"\",\"mac\": \"\",\"ip\": \"\",\"list\": []}";
   
  msg_t msg = {
    .type = MSG_TYPE_MQTT_PUBLISH,
    .from = REG_MGR_CTRL,
    .to = REG_MQTT_CTRL,
  };

  ESP_LOGI(TAG, "++%s()", __func__);
  ESP_LOGD(TAG, "[%s] MAC: %02X:%02X:%02X:%02X:%02X:%02X", __func__, GET_ETH_MAC(mgr_eth_mac));

  if (mgr_send_to_mqtt_fn) {
    cJSON *root = cJSON_Parse(json_str);
    if (root != NULL)
    {
      int ret = -1;

      cJSON_SetValuestring(cJSON_GetObjectItem(root, "uid"), mgr_uid);
      cJSON_SetValuestring(cJSON_GetObjectItem(root, "mac"), mgr_mac);
      cJSON_SetValuestring(cJSON_GetObjectItem(root, "ip"), mgr_ip);

      cJSON *list = cJSON_GetObjectItem(root, "list");
      for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
        cJSON_AddItemToArray(list, cJSON_CreateString(mgr_reg_list[idx].name));
      }
      if ((ret = cJSON_PrintPreallocated(root, msg.payload.mqtt.u.data.msg, DATA_MSG_SIZE, 0)) == 1) {
        sprintf(msg.payload.mqtt.u.data.topic, mgr_reg_pattern);
        esp_err_t result = mgr_send_to_mqtt_fn(&msg);
        if (result != ESP_OK) {
          ESP_LOGE(TAG, "[%s] Send() - Error: %d", __func__, result);
        }
      } else {
        ESP_LOGE(TAG, "[%s] cJSON_PrintPreallocated() - Error: %d", __func__, ret);
      }
      cJSON_Delete(root);

    } else {
      ESP_LOGE(TAG, "[%s] cJSON_Parse() returns NULL", __func__);
    }
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Send message with topic to subscribe for every module
 *
 * topic: STRING format
 *   ESP_12AB34/topic_1 - 1st topic
 *   ESP_12AB34/topic_2 - 2nd topic
 *   ...
 *   ESP_12AB34/topic_n - n topic
 *
 */
void mgr_SubscribeTopic(void) {
  msg_t msg = {
    .type = MSG_TYPE_MQTT_SUBSCRIBE,
    .from = REG_MGR_CTRL,
    .to = REG_MQTT_CTRL,
  };

  ESP_LOGI(TAG, "++%s()", __func__);
  if (mgr_send_to_mqtt_fn) {

    /* Subscribe REGISTER/ESP */
    sprintf(msg.payload.mqtt.u.topic, mgr_reg_pattern);
    esp_err_t result = mgr_send_to_mqtt_fn(&msg);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] Send() - Error: %d", __func__, result);
    }

    /* Subscribe every registered module */
    for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
      mgr_topic_list[idx].type = mgr_reg_list[idx].type;
      sprintf(mgr_topic_list[idx].topic, mgr_topic_pattern, mgr_uid, mgr_reg_list[idx].name);
      sprintf(msg.payload.mqtt.u.topic, "%s", mgr_topic_list[idx].topic);
      esp_err_t result = mgr_send_to_mqtt_fn(&msg);
      if (result != ESP_OK) {
        ESP_LOGE(TAG, "[%s] Send() - Error: %d", __func__, result);
      }
    }
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Create a list of subscribed topics and send it to MQTT
 *
 * json: JSON format
 *   {
 *     "topics": ["ESP_12AB34/cmd/eth", "ESP_12AB34/cmd/mqtt"],
 *   }
 *
 */
void mgr_SubscribeList(void) {
  const char json_str[] = "{ \"topics\": [] }";
  msg_t msg = {
    .type = MSG_TYPE_MQTT_SUBSCRIBE_LIST,
    .from = REG_MGR_CTRL,
    .to = REG_MQTT_CTRL,
  };

  ESP_LOGI(TAG, "++%s()", __func__);
  if (mgr_send_to_mqtt_fn) {
    cJSON *root = cJSON_Parse(json_str);
    if (root != NULL)
    {
      int ret = -1;
      cJSON *list = cJSON_GetObjectItem(root, "topics");

      for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
        mgr_topic_list[idx].type = mgr_reg_list[idx].type;
        sprintf(mgr_topic_list[idx].topic, mgr_topic_pattern, mgr_uid, mgr_reg_list[idx].name);
        cJSON_AddItemToArray(list, cJSON_CreateString(mgr_topic_list[idx].topic));
      }

      if ((ret = cJSON_PrintPreallocated(root, msg.payload.mqtt.u.json, DATA_JSON_SIZE, 0)) == 1) {
        esp_err_t result = mgr_send_to_mqtt_fn(&msg);
        if (result != ESP_OK) {
          ESP_LOGE(TAG, "[%s] Send() - Error: %d", __func__, result);
        }
      } else {
        ESP_LOGE(TAG, "[%s] cJSON_PrintPreallocated() - Error: %d", __func__, ret);
      }
      cJSON_Delete(root);

    } else {
      ESP_LOGE(TAG, "[%s] cJSON_Parse() returns NULL", __func__);
    }
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

void mgr_StartMqtt(void) {
  ESP_LOGI(TAG, "++%s()", __func__);
  if (mgr_send_to_mqtt_fn) {
    msg_t msg = {
      .type = MSG_TYPE_MQTT_START,
      .from = REG_MGR_CTRL,
      .to = REG_MQTT_CTRL,
    };
    esp_err_t result = mgr_send_to_mqtt_fn(&msg);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] Send() - Error: %d", __func__, result);
    }
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

static esp_err_t mgr_parseMqttEvent(data_mqtt_event_e event_id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(event_id: %d [%s])", __func__, event_id, GET_DATA_MQTT_EVENT_NAME(event_id));
  if (event_id == DATA_MQTT_EVENT_CONNECTED) {
    mgr_CreateModuleList();

    mgr_SubscribeTopic();
    //mgr_SubscribeList();
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_ParseRegisterRequest(const data_mqtt_data_t* data_ptr) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(topic: '%s', msg: '%s')", __func__, data_ptr->topic, data_ptr->msg);
  cJSON* root = cJSON_Parse(data_ptr->msg);
  if (root) {
    const cJSON* operation = cJSON_GetObjectItem(root, "operation");
    if (operation) {
      const char* o_str = cJSON_GetStringValue(operation);
      ESP_LOGD(TAG, "[%s] operation: '%s'", __func__, o_str);
      if (strcmp(o_str, "get") == 0) {
        mgr_CreateModuleList();
        result = ESP_OK;
      } else {
        ESP_LOGW(TAG, "[%s] Unknown operation: '%s'", __func__, o_str);
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

static esp_err_t mgr_ParseMqttData(const msg_t* msg) {
  const data_mqtt_data_t* data_ptr = &(msg->payload.mqtt.u.data);
  esp_err_t result = ESP_ERR_NOT_FOUND;

  ESP_LOGI(TAG, "++%s(topic: '%s', msg: '%s')", __func__, data_ptr->topic, data_ptr->msg);

  if (memcmp(data_ptr->topic, mgr_reg_pattern, strlen(mgr_reg_pattern)) == 0) {
    /* This is specific topic = REGISTER/ESP */
    /* When it arrived then resend REGISTER/ESP message once again */
    result = mgr_ParseRegisterRequest(data_ptr);

  } else {
    /* topic must be grater than UID len + at least 4 bytes: */
    /* UID + '/' + module name:  e.g. ESP_12AB34/123 */
    if (strlen(data_ptr->topic) < (MGR_UID_LEN + 4)) {
      ESP_LOGE(TAG, "[%s] topic: '%s' too short, size: %d", __func__, data_ptr->topic, strlen(data_ptr->topic));
      return ESP_ERR_INVALID_SIZE;
    }

    /* Check UID in the topic */
    if (memcmp(data_ptr->topic, mgr_uid, MGR_UID_LEN) != 0) {
      ESP_LOGE(TAG, "[%s] topic: '%s' doesn't contain UID: '%s'", __func__, data_ptr->topic, mgr_uid);
      return ESP_ERR_INVALID_ARG;
    }

    /* Gets module name and call send_fn() if module was found */
    ESP_LOGD(TAG, "[%s] Find a module: '%s'", __func__, &(data_ptr->topic[MGR_UID_LEN + 1]));
    for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
      ESP_LOGD(TAG, "[%s] Registered module: '%s' on idx: %d", __func__, mgr_reg_list[idx].name, idx);
      if (memcmp(&(data_ptr->topic[MGR_UID_LEN + 1]), mgr_reg_list[idx].name, strlen(mgr_reg_list[idx].name)) == 0) {
        ESP_LOGD(TAG, "[%s] Module '%s' found.", __func__, mgr_reg_list[idx].name);
        if (mgr_reg_list[idx].send_fn) {
          result = mgr_reg_list[idx].send_fn(msg);
        } else {
          result = ESP_FAIL;
        }
        break;
      }
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_ParseMsg(const msg_t* msg) {
  esp_err_t result = ESP_FAIL;

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

    case MSG_TYPE_ETH_EVENT: {
      data_eth_event_e event_id = msg->payload.eth.u.event_id;

      ESP_LOGD(TAG, "[%s] Event: %d [%s]", __func__, event_id, GET_DATA_ETH_EVENT_NAME(event_id));
      break;
    }

    case MSG_TYPE_ETH_MAC: {
      const data_eth_mac_t* mac_ptr = &(msg->payload.eth.u.mac);

      /* Store MAC address */
      memcpy(mgr_eth_mac, mac_ptr, sizeof(data_eth_mac_t));
      ESP_LOGD(TAG, "[%s] MAC: %02X:%02X:%02X:%02X:%02X:%02X", __func__, GET_ETH_MAC(mgr_eth_mac));

      mgr_CreateUid();
      break;
    }

    case MSG_TYPE_ETH_IP: {
      uint8_t* addr = NULL;

      mgr_eth_info = msg->payload.eth.u.info;

      addr = (uint8_t*) &(mgr_eth_info.ip);
      sprintf(mgr_ip, mgr_ip_pattern, addr[0], addr[1], addr[2], addr[3]);
      ESP_LOGD(TAG, "[%s]   IP: %d.%d.%d.%d", __func__, 
        addr[0], addr[1], addr[2], addr[3]
      );

      addr = (uint8_t*) &(mgr_eth_info.mask);
      ESP_LOGD(TAG, "[%s] MASK: %d.%d.%d.%d", __func__, 
        addr[0], addr[1], addr[2], addr[3]
      );

      addr = (uint8_t*) &(mgr_eth_info.gw);
      ESP_LOGD(TAG, "[%s]   GW: %d.%d.%d.%d", __func__, 
        addr[0], addr[1], addr[2], addr[3]
      );

      mgr_StartMqtt();
      break;
    }

    case MSG_TYPE_MQTT_EVENT: {
      data_mqtt_event_e event_id = msg->payload.mqtt.u.event_id;

      ESP_LOGD(TAG, "[%s] event_id: %d [%s]", __func__, event_id, GET_DATA_MQTT_EVENT_NAME(event_id));
      result = mgr_parseMqttEvent(event_id);
      break;
    }
    case MSG_TYPE_MQTT_DATA: {
      result = mgr_ParseMqttData(msg);
      break;
    }

    default: {
      break;
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_NotifyCtrl(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(type: %d [%s], from: 0x%08lx, to: 0x%08lx)", __func__, 
      msg->type,  GET_MSG_TYPE_NAME(msg->type),
      msg->from, msg->to);
  for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
    if (msg->to & mgr_reg_list[idx].type) {
      if (mgr_reg_list[idx].send_fn) {
        result = mgr_reg_list[idx].send_fn(msg);
      }
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Manager task's function
 * 
 * @param param 
 */
static void mgr_TaskFn(void* param) {
  msg_t msg;
  bool loop = true;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  memset(&msg, 0x00, sizeof(msg_t));
  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    if(xQueueReceive(mgr_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__, 
          msg.type, GET_MSG_TYPE_NAME(msg.type),
          msg.from, msg.to);

      /* First, parse message in manager */
      if (msg.to & REG_MGR_CTRL) {
        result = mgr_ParseMsg(&msg);
      }

      if (result == ESP_TASK_DONE) {
        loop = false;
        result = ESP_OK;
      }

      /* Now, notify specific (or all) registered controller */
      if (msg.to & (~REG_MGR_CTRL)) {
        result = mgr_NotifyCtrl(&msg);
      }

      if (result != ESP_OK) {
        // TODO - Send Error to the Broker
        ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
      }
    } else {
      ESP_LOGE(TAG, "[%s] Message error.", __func__);
    }
  }
  if (mgr_sem_id) {
    xSemaphoreGive(mgr_sem_id);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Init manager
 * 
 * \return esp_err_t 
 */
esp_err_t MGR_Init(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_MGR_CTRL_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGD(TAG, "[%s] Size of msg_t: %d", __func__, sizeof(msg_t));

  /* Initialization message queue */
  mgr_msg_queue = xQueueCreate(MGR_MSG_MAX, sizeof(msg_t));
  if (mgr_msg_queue == NULL)
  {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  mgr_sem_id = xSemaphoreCreateCounting(1, 0);
  if (mgr_sem_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xSemaphoreCreateCounting() failed.", __func__);
    return ESP_FAIL;
  }
 
  /* Initialization manager thread */
  xTaskCreate(mgr_TaskFn, MGR_TASK_NAME, MGR_TASK_STACK_SIZE, NULL, MGR_TASK_PRIORITY, &mgr_task_id);
  if (mgr_task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }

  ESP_LOGD(TAG, "Modules to register: %d", mgr_modules_cnt);
  for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
    result = mgr_Init(idx);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done manager
 * 
 * \return esp_err_t 
 */
esp_err_t MGR_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  for (int idx = mgr_modules_cnt - 1; idx >= 0; --idx) {
    result = mgr_Done(idx);
  }
  if (mgr_task_id) {
    ESP_LOGD(TAG, "[%s] Task stopped", __func__);
  }
  if (mgr_sem_id) {
    vSemaphoreDelete(mgr_sem_id);
    ESP_LOGD(TAG, "[%s] Semaphore deleted", __func__);
  }
  if (mgr_msg_queue) {
    vQueueDelete(mgr_msg_queue);
    ESP_LOGD(TAG, "[%s] Queue deleted", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run manager
 * 
 * \return esp_err_t 
 */
esp_err_t MGR_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
    result = mgr_Run(idx);
  }

  ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake...", __func__);
  xSemaphoreTake(mgr_sem_id, portMAX_DELAY);
  ESP_LOGD(TAG, "[%s] xSemaphoreTake was released.", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t MGR_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mgr_Send(msg);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
