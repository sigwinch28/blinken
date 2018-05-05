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

esp_err_t led_set();

// Wifi initialisation schmoo.
static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

// Global LED state.
static blinken_t b;

/*
WiFi event handler. Used to manage the connection off-thread.
*/
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
  switch(event->event_id) {
  case SYSTEM_EVENT_STA_START:
    ESP_LOGI(TAG, "WiFi ready. Connecting.");
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

/*
Use WiFi values from sdkconfig and connect.
Uses RAM storage instead of nvram.
*/
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

/*
COAP handler for "/led" PUT requests.

Reads the request body, attempts to parse according to the line protocol,
updates the global LED state, and calls the LED setter.
*/
static void
led_handler_put(coap_context_t *ctx, struct coap_resource_t *resource,
		const coap_endpoint_t *local_interface, coap_address_t *peer,
		coap_pdu_t *request, str *token, coap_pdu_t *response) {
  size_t size;
  unsigned char* data;
  ESP_LOGI(TAG, "PUT /led");

  coap_get_data(request, &size, &data);
  char raw[PARSER_BUF_LEN];
  int len;
  if (size < PARSER_BUF_LEN-2) {
    len = size;
  } else {
    len = PARSER_BUF_LEN-2;
  }
  strncpy(raw, (char*)data, len);
  raw[len] = '\0';

  // Make copy of current values for update
  blinken_t res;
  blinken_copy(&b, &res);
  res.time = 0; // We don't want to preserve time
  
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

/*
COAP handler for "/led" GET.
Returns current global config.
*/
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

/*
Initialisation and loop for COAP server.
 */
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
    coap_register_handler(led_resource, COAP_REQUEST_PUT, led_handler_put);
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

/*
Initialise LEDC hardware.
*/
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

/*
Turn values into frequency/resolution-dependent duties.
Only sets a new value if it differs from the existing.
*/
static inline esp_err_t led_set_duty(value_t val, ledc_channel_t channel, int time) {
  uint32_t duty = val * BLINKEN_MAX_DUTY / VALUE_T_MAX;
  ESP_LOGD(TAG, "Setting LED channel %d to %d (%d duty over %dms)", channel, val,
           duty, time);

  if (time == 0) {
    return ledc_set_duty(BLINKEN_MODE, channel, duty);
  } else {
    return ledc_set_fade_with_time(BLINKEN_MODE, channel, duty, time);
  }
}

static inline esp_err_t led_update_duty(ledc_channel_t channel, int time) {
  ESP_LOGD(TAG, "Updating LED channel %d", channel);
  if (time == 0) {
    return ledc_update_duty(BLINKEN_MODE, channel);
  } else {
    return ledc_fade_start(BLINKEN_MODE, channel, LEDC_FADE_NO_WAIT);
  }
}
    

/*
Set all LED channels based on global config.
Does not set duty if unchanged from previous value.
*/
#define ESP_OK_RETURN(x)		     \
  do {					     \
    esp_err_t res = x;			     \
    if (res != ESP_OK) {		     \
      ESP_LOGE(TAG, "Couldn't update duty"); \
      return res;			     \
    }					     \
  } while(0);

#define ESP_HOLD_ERR(err, x)			\
  do {						\
    if (err == ESP_OK) {			\
      err = x;					\
    }						\
  } while(0);

/*
This function requires error handling due to the following bug:
https://github.com/espressif/esp-idf/issues/1914
*/
esp_err_t led_set(blinken_t *new) {
  ESP_LOGD(TAG, "Setting all LED channels");
  
  ESP_OK_RETURN(led_set_duty(new->red, BLINKEN_CHR_CHANNEL, new->time));
  ESP_OK_RETURN(led_set_duty(new->green, BLINKEN_CHG_CHANNEL, new->time));
  ESP_OK_RETURN(led_set_duty(new->blue, BLINKEN_CHB_CHANNEL, new->time));
  ESP_OK_RETURN(led_set_duty(new->white, BLINKEN_CHW_CHANNEL, new->time));

  ESP_LOGD(TAG, "Updating all LED channels.");
  esp_err_t res = ESP_OK;

  ESP_HOLD_ERR(res, led_update_duty(BLINKEN_CHR_CHANNEL, new->time));
  ESP_HOLD_ERR(res, led_update_duty(BLINKEN_CHG_CHANNEL, new->time));
  ESP_HOLD_ERR(res, led_update_duty(BLINKEN_CHB_CHANNEL, new->time));
  ESP_HOLD_ERR(res, led_update_duty(BLINKEN_CHW_CHANNEL, new->time));

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

void app_main() {
  ESP_ERROR_CHECK( nvs_flash_init() );
  led_init();
  wifi_conn_init();

  xTaskCreate(coap_thread, "coap", 4096, NULL, 5, NULL);
}
