#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "driver/ledc.h"

#include "coap.h"
#include "mdns.h"
#include "nvs_flash.h"

#include <stdio.h>

#include "blinken_main.h"
#include "parser.h"

static const char *TAG = "blinken";

/*******************************************************************************
 * Event handling
 ******************************************************************************/
static EventGroupHandle_t wifi_event_group;
const static int IPV4_CONNECTED_BIT = BIT0;
const static int IPV6_CONNECTED_BIT = BIT1;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
  switch(event->event_id) {
  case SYSTEM_EVENT_STA_START:
    ESP_LOGI(TAG, "WiFi ready. Connecting.");
    esp_wifi_connect();
    break;
#if BLINKEN_IPV6
  case SYSTEM_EVENT_STA_CONNECTED:
    ESP_LOGD(TAG, "STA connected. Enabling IPv6.");
    tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
    break;
#endif
  case SYSTEM_EVENT_STA_GOT_IP:
    ESP_LOGI(TAG, "Got WiFi IP: %s",
	     ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
    xEventGroupSetBits(wifi_event_group, IPV4_CONNECTED_BIT);
    break;
  case SYSTEM_EVENT_AP_STA_GOT_IP6:
    ESP_LOGI(TAG, "Got WiFi IPv6: %s",
	     ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip));
    xEventGroupSetBits(wifi_event_group, IPV6_CONNECTED_BIT);
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    ESP_LOGI(TAG, "WiFi disconnected. Reconnecting.");
    esp_wifi_connect();
    xEventGroupClearBits(wifi_event_group, IPV4_CONNECTED_BIT | IPV6_CONNECTED_BIT);
    break;
  default:
    break;
  }
  mdns_handle_system_event(ctx, event);
  return ESP_OK;
}

/*******************************************************************************
 * WiFi
 ******************************************************************************/
static void wifi_conn_init() {
  ESP_LOGI(TAG, "Connecting to WiFi");
  
  tcpip_adapter_init();
  wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK( esp_event_loop_init(wifi_event_handler, NULL) );
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

  wifi_config_t wifi_config = {
    .sta = {
      .ssid = BLINKEN_WIFI_SSID,
      .password = BLINKEN_WIFI_PASSWORD,
    },
  };
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
  ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
  ESP_ERROR_CHECK( esp_wifi_start() );

  ESP_LOGI(TAG, "Connected to WiFI. SSID: %s, Password: %s",
	   BLINKEN_WIFI_SSID, BLINKEN_WIFI_PASSWORD);
}

/*******************************************************************************
 * LED control
 ******************************************************************************/
static blinken_t b;

static void led_init() {
  ESP_LOGI(TAG, "Initialising LED PWM");

  // Clear current configuration, ready for future updates via COAP
  blinken_init(&b);

  ESP_LOGD(TAG, "Configuring PWM timer");
  ledc_timer_config_t ledc_timer = {
      .duty_resolution = BLINKEN_RESOLUTION, // resolution of PWM duty
      .freq_hz = BLINKEN_PWM_HZ,             // frequency of PWM signal
      .speed_mode = BLINKEN_MODE,            // timer mode
      .timer_num = BLINKEN_TIMER             // timer index
  };
  ledc_timer_config(&ledc_timer);

  struct {
    int gpio_num;
    ledc_channel_t channel;
  } channels[BLINKEN_CH_NUM] = {
    {.channel  = BLINKEN_CHR_CHANNEL,
     .gpio_num = BLINKEN_CHR_GPIO
    },
    {.channel  = BLINKEN_CHG_CHANNEL,
     .gpio_num = BLINKEN_CHG_GPIO
    },
    {.channel  = BLINKEN_CHB_CHANNEL,
     .gpio_num = BLINKEN_CHB_GPIO
    },
    {.channel  = BLINKEN_CHW_CHANNEL,
     .gpio_num = BLINKEN_CHW_GPIO
    },
  };

  ledc_channel_config_t ch = {
    .duty = 0,
    .speed_mode = BLINKEN_MODE,
    .timer_sel = BLINKEN_TIMER
  };
  
  for (int i = 0; i < BLINKEN_CH_NUM; i++) {
    ch.channel = channels[i].channel;
    ch.gpio_num = channels[i].gpio_num;
    ESP_LOGD(TAG,
	     "Init LED channel. channel=%d, gpio_num=%d, duty=%d, speed_mode=%d, timer_sel=%d",
	     ch.channel, ch.gpio_num, ch.duty, ch.speed_mode, ch.timer_sel);
    ledc_channel_config(&ch);
  }

  ESP_LOGD(TAG, "Init hardware PWM fading.");
  ledc_fade_func_install(0);
}

static inline esp_err_t led_set_duty(ledc_channel_t channel, value_t val, int time) {
  uint32_t duty = val * BLINKEN_MAX_DUTY / VALUE_T_MAX;
  ESP_LOGD(TAG, "Setting LED channel. channel=%d, val=%d, time=%d, duty=%d",
	   channel, val, time, duty);

  return ledc_set_fade_with_time(BLINKEN_MODE, channel, duty, time);
}

static inline esp_err_t led_update_duty(ledc_channel_t channel) {
  ESP_LOGD(TAG, "Updating LED channel. channel=%d", channel);
  return ledc_fade_start(BLINKEN_MODE, channel, LEDC_FADE_NO_WAIT);
}

esp_err_t led_set_channel(ledc_channel_t channel, value_t val, int time) {
  esp_err_t res = led_set_duty(channel, val, time);
  if (res != ESP_OK) {
    return res;
  }
  return led_update_duty(channel);
}

esp_err_t led_set(blinken_t *new) {
  
  ESP_LOGD(TAG, "Updating all LED channels.");
  esp_err_t res = ESP_OK;

  ESP_HOLD_ERR(res, led_set_channel(BLINKEN_CHR_CHANNEL, new->red,   new->time));
  ESP_HOLD_ERR(res, led_set_channel(BLINKEN_CHG_CHANNEL, new->green, new->time));
  ESP_HOLD_ERR(res, led_set_channel(BLINKEN_CHB_CHANNEL, new->blue,  new->time));
  ESP_HOLD_ERR(res, led_set_channel(BLINKEN_CHW_CHANNEL, new->white, new->time));

  if (res != ESP_OK && new != &b) {
    ESP_LOGE(TAG, "Couldn't set all duties. reverting.");
    b.time = 0;
    led_set(&b);
    return res;
  }

  if (new != &b) {
    blinken_copy(new, &b);
  }
  return res;
}

/*******************************************************************************
 * COAP
 ******************************************************************************/
static void
led_handler_put(coap_context_t *ctx, struct coap_resource_t *resource,
		const coap_endpoint_t *local_interface, coap_address_t *peer,
		coap_pdu_t *request, str *token, coap_pdu_t *response) {
  size_t size;
  unsigned char* data;
  ESP_LOGI(TAG, "PUT /led");

  // Copy and null-terminate the data.
  coap_get_data(request, &size, &data);
  char raw[PARSER_BUF_LEN];
  int len;
  if (size < PARSER_BUF_LEN-2) {
    len = size;
  } else {
    len = PARSER_BUF_LEN-2;
  }
  strncpy(raw, (char*)data, len);
  // If the copied data is of length `len`, it is not null-terminated, so
  // we must do it ourselves.
  raw[len] = '\0';

  // Make copy of current values for update
  blinken_t res;
  blinken_copy(&b, &res);
  res.time = 0; // We don't want to preserve time

  // Parse the payload
  char *ptr = blinken_parse(&res, raw);
  
  if (ptr != raw) {
    ESP_LOGD(TAG, "Setting LEDs: %s", raw);
    // Update global config and set LEDs
    if (led_set(&res) == ESP_OK) {
      ESP_LOGD(TAG, "LED update successful.");
      resource->dirty = 1;
      response->hdr->code = COAP_RESPONSE_CODE(204);
    } else {
      ESP_LOGE(TAG, "Couldn't set LEDs using provided values.");
      response->hdr->code = COAP_RESPONSE_CODE(400);
    }
  } else {
    ESP_LOGE(TAG, "Invalid payload: %s", raw);
    response->hdr->code = COAP_RESPONSE_CODE(400);
  }
}

static void
led_handler_get(coap_context_t *ctx, struct coap_resource_t *resource,
		const coap_endpoint_t *local_interface, coap_address_t *peer,
		coap_pdu_t *request, str *token, coap_pdu_t *response)
{
  ESP_LOGI(TAG, "GET /led");
  unsigned char buf[3];

  // Serialize current config
  char data[PARSER_BUF_LEN];
  char *ptr = data;
  int len = blinken_snprint(&ptr, PARSER_BUF_LEN, &b);

  // Set response code and send payload
  response->hdr->code = COAP_RESPONSE_CODE(205);
  coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
  coap_add_data(response, len, (unsigned char*)data);
}

static void coap_task(void *p) {
  coap_context_t *ctx;
  coap_address_t serv_addr;
  coap_resource_t *led_resource;
  fd_set readfds;
  
  ESP_LOGD(TAG, "Starting COAP server. Waiting for WiFi...");
  xEventGroupWaitBits(wifi_event_group, IPV4_CONNECTED_BIT | IPV6_CONNECTED_BIT,
		      false, true, portMAX_DELAY);
  ESP_LOGD(TAG, "WiFi connected. Continuing with COAP server startup.");

  coap_address_init(&serv_addr);
#if BLINKEN_IPV6
  serv_addr.addr.sin6.sin6_family = AF_INET6;
  serv_addr.addr.sin6.sin6_addr = in6addr_any;
  serv_addr.addr.sin6.sin6_port = htons(COAP_DEFAULT_PORT);
  serv_addr.size = sizeof(serv_addr.addr.sin6);
#else
  serv_addr.addr.sin.sin_family = AF_INET;
  serv_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;
  serv_addr.addr.sin.sin_port = htons(COAP_DEFAULT_PORT);
#endif

  ctx = coap_new_context(&serv_addr);

  if (ctx) {
    ESP_LOGD(TAG, "Creating COAP resource for GET \"/%s\".", BLINKEN_RESOURCE);
    led_resource = coap_resource_init((unsigned char *)BLINKEN_RESOURCE,
				      strlen(BLINKEN_RESOURCE), 0);
    
    coap_register_handler(led_resource, COAP_REQUEST_GET, led_handler_get);
    coap_register_handler(led_resource, COAP_REQUEST_PUT, led_handler_put);
    coap_add_resource(ctx, led_resource);

    ESP_LOGI(TAG, "COAP server started.");

    while(1) {
      FD_ZERO(&readfds);
      FD_CLR(ctx->sockfd, &readfds);
      FD_SET(ctx->sockfd, &readfds);
      int result = select(ctx->sockfd+1, &readfds, 0, 0, NULL);
      if (result > 0 && FD_ISSET(ctx->sockfd, &readfds)) {
	ESP_LOGD(TAG, "Handling incoming COAP request.");
	coap_read(ctx);
      } else if (result < 0) {
	ESP_LOGE(TAG, "COAP socket error.");
	break;
      }
    }

    ESP_LOGD(TAG, "Cleaning up COAP context.");
    coap_free_context(ctx);
  } else {
    ESP_LOGE(TAG, "Couldn't create COAP context.");
  }
  
  vTaskDelete(NULL);
}

/*******************************************************************************
 * MDNS
 ******************************************************************************/

static void app_mdns_init() {
  ESP_LOGD(TAG, "Advertising mDNS services");

  ESP_ERROR_CHECK( mdns_init() );
  ESP_ERROR_CHECK( mdns_hostname_set(BLINKEN_HOSTNAME) );
  ESP_ERROR_CHECK( mdns_instance_name_set(BLINKEN_INSTANCE) );

  mdns_txt_item_t serviceTxtData[2] = {
    {"path", "/led"},
    {"location", "window"},
  };

  ESP_ERROR_CHECK( mdns_service_add(NULL, "_blinken", "_udp", COAP_DEFAULT_PORT, serviceTxtData, 2) );
}

/*******************************************************************************
 * Main
 ******************************************************************************/
void app_main() {
  ESP_ERROR_CHECK( nvs_flash_init() );
  led_init();
  wifi_conn_init();
  app_mdns_init();

  xTaskCreate(coap_task, "coap", 4096, NULL, 5, NULL);
}
