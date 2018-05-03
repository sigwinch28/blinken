#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "driver/ledc.h"

#include "coap.h"
#include "nvs_flash.h"

#include <stdio.h>

static const char *TAG = "blinken";

#include "blinken_main.h"
#include "parser.h"

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

static blinken_t b;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
  switch(event->event_id) {
  case SYSTEM_EVENT_STA_START:
    esp_wifi_connect();
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    ESP_LOGI(TAG, "Got WiFi IP: %s",
	     ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    /* This is a workaround as ESP32 WiFi libs don't currently
       auto-reassociate. */
    ESP_LOGI(TAG, "WiFi disconnected. Reconnecting.");
    esp_wifi_connect();
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    break;
  default:
    break;
  }
  return ESP_OK;
}

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

static void
led_handler_get(coap_context_t *ctx, struct coap_resource_t *resource,
		const coap_endpoint_t *local_interface, coap_address_t *peer,
		coap_pdu_t *request, str *token, coap_pdu_t *response)
{
  unsigned char buf[3];
  char data[PARSER_BUF_LEN];
  char *ptr = data;
  int len = blinken_snprint(&ptr, PARSER_BUF_LEN, &b);
  response->hdr->code = COAP_RESPONSE_CODE(205);
  coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
  coap_add_data(response, len, (unsigned char*)data);
}

static void coap_thread(void *p) {
  coap_context_t *ctx;
  coap_address_t serv_addr;
  coap_resource_t *led_resource;
  fd_set readfds;
  
  ESP_LOGI(TAG, "Starting COAP server. Waiting for WiFi...");
  xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
		      false, true, portMAX_DELAY);
  ESP_LOGI(TAG, "WiFi connected. Continuing with COAP server startup.");
  coap_address_init(&serv_addr);
  serv_addr.addr.sin.sin_family = AF_INET;
  serv_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;
  serv_addr.addr.sin.sin_port = htons(COAP_DEFAULT_PORT);
  ctx = coap_new_context(&serv_addr);

  if (ctx) {
    ESP_LOGD(TAG, "Creating COAP resource for GET led");
    led_resource = coap_resource_init((unsigned char *)"led", 3, 0);
    coap_register_handler(led_resource, COAP_REQUEST_GET, led_handler_get);
    coap_add_resource(ctx, led_resource);

    while(1) {
      FD_ZERO(&readfds);
      FD_CLR(ctx->sockfd, &readfds);
      FD_SET(ctx->sockfd, &readfds);
      int result = select(ctx->sockfd+1, &readfds, 0, 0, NULL);
      if (result > 0 && FD_ISSET(ctx->sockfd, &readfds)) {
	ESP_LOGD(TAG, "Handling incoming COAP request");
	coap_read(ctx);
      } else if (result < 0) {
	ESP_LOGE(TAG, "COAP socket error.");
	break;
      }
    }

    ESP_LOGD(TAG, "Cleaning up COAP context");
    coap_free_context(ctx);
  } else {
    ESP_LOGE(TAG, "Couldn't create COAP context.");
  }
  
  vTaskDelete(NULL);
}

static void led_init() {
  blinken_init(&b);
  
  ESP_LOGI(TAG, "Initialising LED PWM");

  ESP_LOGD(TAG, "Configuring PWM timer");
  ledc_timer_config_t ledc_timer = {
      .duty_resolution = BLINKEN_RESOLUTION, // resolution of PWM duty
      .freq_hz = BLINKEN_PWM_HZ,             // frequency of PWM signal
      .speed_mode = BLINKEN_MODE,            // timer mode
      .timer_num = BLINKEN_TIMER             // timer index
  };
  ledc_timer_config(&ledc_timer);

  ESP_LOGD(TAG, "Setting up PWM channels");
  ledc_channel_config_t ledc_channel[BLINKEN_CH_NUM] = {
      {.channel = BLINKEN_CHR_CHANNEL,
       .duty = 0,
       .gpio_num = BLINKEN_CHR_GPIO,
       .speed_mode = BLINKEN_MODE,
       .timer_sel = BLINKEN_TIMER},
      {.channel = BLINKEN_CHG_CHANNEL,
       .duty = 0,
       .gpio_num = BLINKEN_CHG_GPIO,
       .speed_mode = BLINKEN_MODE,
       .timer_sel = BLINKEN_TIMER},
      {.channel = BLINKEN_CHB_CHANNEL,
       .duty = 0,
       .gpio_num = BLINKEN_CHB_GPIO,
       .speed_mode = BLINKEN_MODE,
       .timer_sel = BLINKEN_TIMER},
      {.channel = BLINKEN_CHW_CHANNEL,
       .duty = 0,
       .gpio_num = BLINKEN_CHW_GPIO,
       .speed_mode = BLINKEN_MODE,
       .timer_sel = BLINKEN_TIMER},
  };

  for (int ch = 0; ch < BLINKEN_CH_NUM; ch++) {
    ledc_channel_config(&ledc_channel[ch]);
  }

  ESP_LOGD(TAG, "Setting up PWM hardware fading");
  ledc_fade_func_install(0);
}

static inline void led_set_channel(value_t val, ledc_channel_t channel, int time) {
  uint32_t duty = val * BLINKEN_MAX_DUTY / VALUE_T_MAX;
  ESP_LOGI(TAG, "Setting LED channel %d to %d (%d duty over %dms)", channel, val,
           duty, time);
  ledc_set_fade_with_time(BLINKEN_MODE, channel, duty, time);
  ledc_fade_start(BLINKEN_MODE, channel, LEDC_FADE_NO_WAIT);
}

void led_set() {
  ESP_LOGD(TAG, "Setting all LED channels");
  led_set_channel(b.red, BLINKEN_CHR_CHANNEL, b.time);
  led_set_channel(b.green, BLINKEN_CHG_CHANNEL, b.time);
  led_set_channel(b.blue, BLINKEN_CHB_CHANNEL, b.time);
  led_set_channel(b.white, BLINKEN_CHW_CHANNEL, b.time);
}

void app_main() {
  ESP_ERROR_CHECK( nvs_flash_init() );
  led_init();
  wifi_conn_init();

  xTaskCreate(coap_thread, "coap", 2048, NULL, 5, NULL);
}
