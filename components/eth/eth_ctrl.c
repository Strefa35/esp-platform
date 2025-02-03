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
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_eth_driver.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_check.h"

#include "sdkconfig.h"

#include "tags.h"

#include "msg.h"
#include "eth_ctrl.h"
#include "manager.h"

#if CONFIG_ETH_CTRL_USE_SPI_ETHERNET
  #include "driver/spi_master.h"
#endif // CONFIG_ETH_CTRL_USE_SPI_ETHERNET

static const char* TAG = ETH_CTRL_TAG;


#if CONFIG_ETH_CTRL_SPI_ETHERNETS_NUM
  #define SPI_ETHERNETS_NUM           CONFIG_EXAMPLE_SPI_ETHERNETS_NUM
#else
  #define SPI_ETHERNETS_NUM           0
#endif

#if CONFIG_ETH_CTRL_USE_INTERNAL_ETHERNET
  #define INTERNAL_ETHERNETS_NUM      1
#else
  #define INTERNAL_ETHERNETS_NUM      0
#endif

#define INIT_SPI_ETH_MODULE_CONFIG(eth_module_config, num)                                \
  do {                                                                                    \
    eth_module_config[num].spi_cs_gpio = CONFIG_EXAMPLE_ETH_SPI_CS ##num## _GPIO;         \
    eth_module_config[num].int_gpio = CONFIG_EXAMPLE_ETH_SPI_INT ##num## _GPIO;           \
    eth_module_config[num].polling_ms = CONFIG_EXAMPLE_ETH_SPI_POLLING ##num## _MS;       \
    eth_module_config[num].phy_reset_gpio = CONFIG_EXAMPLE_ETH_SPI_PHY_RST ##num## _GPIO; \
    eth_module_config[num].phy_addr = CONFIG_EXAMPLE_ETH_SPI_PHY_ADDR ##num;              \
  } while(0)

typedef struct {
  uint8_t spi_cs_gpio;
  int8_t int_gpio;
  uint32_t polling_ms;
  int8_t phy_reset_gpio;
  uint8_t phy_addr;
  uint8_t *mac_addr;
} spi_eth_module_config_t;


#if CONFIG_ETH_CTRL_USE_SPI_ETHERNET
  static bool gpio_isr_svc_init_by_eth = false; // indicates that we initialized the GPIO ISR service
#endif // CONFIG_ETH_CTRL_USE_SPI_ETHERNET

static esp_eth_handle_t *eth_ctrl_handles = NULL;
static uint8_t eth_ctrl_cnt = 0;

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
  uint8_t mac_addr[6] = {0};
  /* we can get the ethernet driver handle from event data */
  esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
  msg_t msg = { .type = MSG_TYPE_ETH_EVENT, .from = MSG_ETH_CTRL, .to = MSG_ALL_CTRL };
  bool send = true;

  switch (event_id) {
    case ETHERNET_EVENT_CONNECTED: {
      esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
      ESP_LOGI(TAG, "Ethernet Link Up");
      ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
      sprintf(msg.data.eth.event, "%s", "ETH CONNECTED");
      break;
    }
    case ETHERNET_EVENT_DISCONNECTED: {
      ESP_LOGI(TAG, "Ethernet Link Down");
      sprintf(msg.data.eth.event, "%s", "ETH DISCONNECTED");
      break;
    }
    case ETHERNET_EVENT_START: {
      ESP_LOGI(TAG, "Ethernet Started");
      sprintf(msg.data.eth.event, "%s", "ETH STARTED");
      break;
    }
    case ETHERNET_EVENT_STOP: {
      ESP_LOGI(TAG, "Ethernet Stopped");
      sprintf(msg.data.eth.event, "%s", "ETH STOPPED");
      break;
    }
    default: {
      send = false;
      break;
    }
  }
  if (send) {
    esp_err_t result = MGR_Send(&msg);
    ESP_LOGI(TAG, "MSG_Send() - result: %d", result);
  }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
  ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
  const esp_netif_ip_info_t *ip_info = &event->ip_info;
  msg_t msg = { .type = MSG_TYPE_ETH_IP, .from = MSG_ETH_CTRL, .to = MSG_MGR_CTRL };

  sprintf(msg.data.eth.ip, IPSTR, IP2STR(&ip_info->ip));
  sprintf(msg.data.eth.mask, IPSTR, IP2STR(&ip_info->netmask));
  sprintf(msg.data.eth.gw, IPSTR, IP2STR(&ip_info->gw));

  ESP_LOGI(TAG, "Ethernet Got IP Address");
  ESP_LOGI(TAG, "~~~~~~~~~~~");
  ESP_LOGI(TAG, "ETH IP: " IPSTR, IP2STR(&ip_info->ip));
  ESP_LOGI(TAG, "ETH MASK: " IPSTR, IP2STR(&ip_info->netmask));
  ESP_LOGI(TAG, "ETH GW: " IPSTR, IP2STR(&ip_info->gw));
  ESP_LOGI(TAG, "~~~~~~~~~~~");

  esp_err_t result = MGR_Send(&msg);
  ESP_LOGI(TAG, "MSG_Send() - result: %d", result);
}

#ifdef CONFIG_ETH_CTRL_USE_INTERNAL_ETHERNET
/**
 * @brief Internal ESP32 Ethernet initialization
 *
 * @param[out] mac_out optionally returns Ethernet MAC object
 * @param[out] phy_out optionally returns Ethernet PHY object
 * @return
 *          - esp_eth_handle_t if init succeeded
 *          - NULL if init failed
 */
static esp_eth_handle_t ethctrl_InitInternal(esp_eth_mac_t **mac_out, esp_eth_phy_t **phy_out)
{
  esp_eth_handle_t ret = NULL;

  ESP_LOGI(TAG, "++%s()", __func__);

  // Init common MAC and PHY configs to default
  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

  // Update PHY config based on board specific configuration
  phy_config.phy_addr = CONFIG_ETH_CTRL_PHY_ADDR;
  phy_config.reset_gpio_num = CONFIG_ETH_CTRL_PHY_RST_GPIO;
  // Init vendor specific MAC config to default
  eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
  // Update vendor specific MAC config based on board configuration
  esp32_emac_config.smi_gpio.mdc_num = CONFIG_ETH_CTRL_MDC_GPIO;
  esp32_emac_config.smi_gpio.mdio_num = CONFIG_ETH_CTRL_MDIO_GPIO;
#if CONFIG_ETH_CTRL_USE_SPI_ETHERNET
  // The DMA is shared resource between EMAC and the SPI. Therefore, adjust
  // EMAC DMA burst length when SPI Ethernet is used along with EMAC.
  esp32_emac_config.dma_burst_len = ETH_DMA_BURST_LEN_4;
#endif // CONFIG_ETH_CTRL_USE_SPI_ETHERNET
  // Create new ESP32 Ethernet MAC instance
  esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
  // Create new PHY instance based on board configuration
#if CONFIG_ETH_CTRL_PHY_GENERIC
  esp_eth_phy_t *phy = esp_eth_phy_new_generic(&phy_config);
#elif CONFIG_ETH_CTRL_PHY_IP101
  esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);
#elif CONFIG_ETH_CTRL_PHY_RTL8201
  esp_eth_phy_t *phy = esp_eth_phy_new_rtl8201(&phy_config);
#elif CONFIG_ETH_CTRL_PHY_LAN87XX
  esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
#elif CONFIG_ETH_CTRL_PHY_DP83848
  esp_eth_phy_t *phy = esp_eth_phy_new_dp83848(&phy_config);
#elif CONFIG_ETH_CTRL_PHY_KSZ80XX
  esp_eth_phy_t *phy = esp_eth_phy_new_ksz80xx(&phy_config);
#endif
  // Init Ethernet driver to default and install it
  esp_eth_handle_t eth_handle = NULL;
  esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
  ESP_GOTO_ON_FALSE(esp_eth_driver_install(&config, &eth_handle) == ESP_OK, NULL,
                      err, TAG, "Ethernet driver install failed");

  if (mac_out != NULL) {
      *mac_out = mac;
  }
  if (phy_out != NULL) {
      *phy_out = phy;
  }
  return eth_handle;
err:
  if (eth_handle != NULL) {
      esp_eth_driver_uninstall(eth_handle);
  }
  if (mac != NULL) {
      mac->del(mac);
  }
  if (phy != NULL) {
      phy->del(phy);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
  return ret;
}
#endif // CONFIG_ETH_CTRL_USE_INTERNAL_ETHERNET

static esp_err_t ethctrl_InitEthernet(esp_eth_handle_t *eth_handles_out[], uint8_t *eth_cnt_out)
{
  esp_err_t ret = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

#if CONFIG_ETH_CTRL_USE_INTERNAL_ETHERNET || CONFIG_ETH_CTRL_USE_SPI_ETHERNET
  ESP_GOTO_ON_FALSE(eth_handles_out != NULL && eth_cnt_out != NULL, ESP_ERR_INVALID_ARG,
                      err, TAG, "invalid arguments: initialized handles array or number of interfaces");
  eth_ctrl_handles = calloc(SPI_ETHERNETS_NUM + INTERNAL_ETHERNETS_NUM, sizeof(esp_eth_handle_t));
  ESP_GOTO_ON_FALSE(eth_ctrl_handles != NULL, ESP_ERR_NO_MEM, err, TAG, "No memory");

  #if CONFIG_ETH_CTRL_USE_INTERNAL_ETHERNET
    eth_ctrl_handles[eth_ctrl_cnt] = ethctrl_InitInternal(NULL, NULL);
    ESP_GOTO_ON_FALSE(eth_ctrl_handles[eth_ctrl_cnt], ESP_FAIL, err, TAG, "Internal Ethernet init failed");
    eth_ctrl_cnt++;
  #endif //CONFIG_ETH_CTRL_USE_INTERNAL_ETHERNET

  #if CONFIG_ETH_CTRL_USE_SPI_ETHERNET
    ESP_GOTO_ON_ERROR(spi_bus_init(), err, TAG, "SPI bus init failed");
    // Init specific SPI Ethernet module configuration from Kconfig (CS GPIO, Interrupt GPIO, etc.)
    spi_eth_module_config_t spi_eth_module_config[CONFIG_ETH_CTRL_SPI_ETHERNETS_NUM] = { 0 };
    INIT_SPI_ETH_MODULE_CONFIG(spi_eth_module_config, 0);
    // The SPI Ethernet module(s) might not have a burned factory MAC address, hence use manually configured address(es).
    // In this example, Locally Administered MAC address derived from ESP32x base MAC address is used.
    // Note that Locally Administered OUI range should be used only when testing on a LAN under your control!
    uint8_t base_mac_addr[ETH_ADDR_LEN];
    ESP_GOTO_ON_ERROR(esp_efuse_mac_get_default(base_mac_addr), err, TAG, "get EFUSE MAC failed");
    uint8_t local_mac_1[ETH_ADDR_LEN];
    esp_derive_local_mac(local_mac_1, base_mac_addr);
    spi_eth_module_config[0].mac_addr = local_mac_1;
    
    #if CONFIG_ETH_CTRL_SPI_ETHERNETS_NUM > 1
      INIT_SPI_ETH_MODULE_CONFIG(spi_eth_module_config, 1);
      uint8_t local_mac_2[ETH_ADDR_LEN];
      base_mac_addr[ETH_ADDR_LEN - 1] += 1;
      esp_derive_local_mac(local_mac_2, base_mac_addr);
      spi_eth_module_config[1].mac_addr = local_mac_2;
    #endif

    #if CONFIG_ETH_CTRL_SPI_ETHERNETS_NUM > 2
      #error Maximum number of supported SPI Ethernet devices is currently limited to 2 by this example.
    #endif

    for (int i = 0; i < CONFIG_ETH_CTRL_SPI_ETHERNETS_NUM; i++) {
        eth_ctrl_handles[eth_ctrl_cnt] = ethctrl_InitSpi(&spi_eth_module_config[i], NULL, NULL);
        ESP_GOTO_ON_FALSE(eth_ctrl_handles[eth_ctrl_cnt], ESP_FAIL, err, TAG, "SPI Ethernet init failed");
        eth_ctrl_cnt++;
    }
  #endif // CONFIG_ETH_CTRL_USE_SPI_ETHERNET

#else
    ESP_LOGD(TAG, "No Ethernet device selected to init");
#endif // CONFIG_ETH_CTRL_USE_INTERNAL_ETHERNET || CONFIG_ETH_CTRL_USE_SPI_ETHERNET

    *eth_handles_out = eth_ctrl_handles;
    *eth_cnt_out = eth_ctrl_cnt;

    return ret;
#if CONFIG_ETH_CTRL_USE_INTERNAL_ETHERNET || CONFIG_ETH_CTRL_USE_SPI_ETHERNET
err:
    free(eth_ctrl_handles);
    ESP_LOGI(TAG, "--%s() - result: %d", __func__, ret);
    return ret;
#endif
}

static esp_err_t ethctrl_DeInitEthernet(esp_eth_handle_t *eth_ctrl_handles, uint8_t eth_ctrl_cnt)
{
    ESP_RETURN_ON_FALSE(eth_ctrl_handles != NULL, ESP_ERR_INVALID_ARG, TAG, "array of Ethernet handles cannot be NULL");
    for (int i = 0; i < eth_ctrl_cnt; i++) {
        esp_eth_mac_t *mac = NULL;
        esp_eth_phy_t *phy = NULL;
        if (eth_ctrl_handles[i] != NULL) {
            esp_eth_get_mac_instance(eth_ctrl_handles[i], &mac);
            esp_eth_get_phy_instance(eth_ctrl_handles[i], &phy);
            ESP_RETURN_ON_ERROR(esp_eth_driver_uninstall(eth_ctrl_handles[i]), TAG, "Ethernet %p uninstall failed", eth_ctrl_handles[i]);
        }
        if (mac != NULL) {
            mac->del(mac);
        }
        if (phy != NULL) {
            phy->del(phy);
        }
    }
#if CONFIG_ETH_CTRL_USE_SPI_ETHERNET
    spi_bus_free(CONFIG_ETH_CTRL_ETH_SPI_HOST);
#if (CONFIG_ETH_CTRL_ETH_SPI_INT0_GPIO >= 0) || (CONFIG_ETH_CTRL_ETH_SPI_INT1_GPIO > 0)
    // We installed the GPIO ISR service so let's uninstall it too.
    // BE CAREFUL HERE though since the service might be used by other functionality!
    if (gpio_isr_svc_init_by_eth) {
        ESP_LOGW(TAG, "uninstalling GPIO ISR service!");
        gpio_uninstall_isr_service();
    }
#endif
#endif //CONFIG_ETH_CTRL_USE_SPI_ETHERNET
    free(eth_ctrl_handles);
    return ESP_OK;
}

static esp_err_t ethctrl_Init(void) {
  uint8_t eth_port_cnt = 0;
  esp_eth_handle_t *eth_ctrl_handles;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  // Initialize Ethernet driver
  ESP_ERROR_CHECK(ethctrl_InitEthernet(&eth_ctrl_handles, &eth_port_cnt));

  esp_netif_t *eth_netifs[eth_port_cnt];
  esp_eth_netif_glue_handle_t eth_netif_glues[eth_port_cnt];

  // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
  ESP_ERROR_CHECK(esp_netif_init());
  // Create default event loop that running in background
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Create instance(s) of esp-netif for Ethernet(s)
  if (eth_port_cnt == 1) {
    // Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and you don't need to modify
    // default esp-netif configuration parameters.
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    eth_netifs[0] = esp_netif_new(&cfg);
    eth_netif_glues[0] = esp_eth_new_netif_glue(eth_ctrl_handles[0]);
    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netifs[0], eth_netif_glues[0]));
  } else {
    // Use ESP_NETIF_INHERENT_DEFAULT_ETH when multiple Ethernet interfaces are used and so you need to modify
    // esp-netif configuration parameters for each interface (name, priority, etc.).
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
    esp_netif_config_t cfg_spi = {
        .base = &esp_netif_config,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
    };
    char if_key_str[10];
    char if_desc_str[10];
    char num_str[3];
    for (int i = 0; i < eth_port_cnt; i++) {
      itoa(i, num_str, 10);
      strcat(strcpy(if_key_str, "ETH_"), num_str);
      strcat(strcpy(if_desc_str, "eth"), num_str);
      esp_netif_config.if_key = if_key_str;
      esp_netif_config.if_desc = if_desc_str;
      esp_netif_config.route_prio -= i*5;
      eth_netifs[i] = esp_netif_new(&cfg_spi);
      eth_netif_glues[i] = esp_eth_new_netif_glue(eth_ctrl_handles[0]);
      // Attach Ethernet driver to TCP/IP stack
      ESP_ERROR_CHECK(esp_netif_attach(eth_netifs[i], eth_netif_glues[i]));
    }
  }

  // Register user defined event handers
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

  // Start Ethernet driver state machine
  for (int i = 0; i < eth_port_cnt; i++) {
    ESP_ERROR_CHECK(esp_eth_start(eth_ctrl_handles[i]));
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t ethctrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  ESP_RETURN_ON_FALSE(eth_ctrl_handles != NULL, ESP_ERR_INVALID_ARG, TAG, "array of Ethernet handles cannot be NULL");
  for (int i = 0; i < eth_ctrl_cnt; i++) {
      esp_eth_mac_t *mac = NULL;
      esp_eth_phy_t *phy = NULL;
      if (eth_ctrl_handles[i] != NULL) {
          esp_eth_get_mac_instance(eth_ctrl_handles[i], &mac);
          esp_eth_get_phy_instance(eth_ctrl_handles[i], &phy);
          ESP_RETURN_ON_ERROR(esp_eth_driver_uninstall(eth_ctrl_handles[i]), TAG, "Ethernet %p uninstall failed", eth_ctrl_handles[i]);
      }
      if (mac != NULL) {
          mac->del(mac);
      }
      if (phy != NULL) {
          phy->del(phy);
      }
  }
#if CONFIG_ETH_CTRL_USE_SPI_ETHERNET
  spi_bus_free(CONFIG_ETH_CTRL_SPI_HOST);
#if (CONFIG_ETH_CTRL_SPI_INT0_GPIO >= 0) || (CONFIG_ETH_CTRL_SPI_INT1_GPIO > 0)
  // We installed the GPIO ISR service so let's uninstall it too.
  // BE CAREFUL HERE though since the service might be used by other functionality!
  if (gpio_isr_svc_init_by_eth) {
      ESP_LOGW(TAG, "uninstalling GPIO ISR service!");
      gpio_uninstall_isr_service();
  }
#endif
#endif //CONFIG_ETH_CTRL_USE_SPI_ETHERNET

  free(eth_ctrl_handles);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}


/**
 * @brief Init Eth controller
 * 
 * \return esp_err_t 
 */
esp_err_t EthCtrl_Init(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_ETH_CTRL_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  ethctrl_Init();
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
  ethctrl_Done();
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
