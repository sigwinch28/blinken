#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

static const char* TAG = "blinken";
#include "esp_log.h"

typedef char value_t;
#define VALUE_T_MAX CHAR_MAX

typedef struct {
  value_t red, green, blue, white;
  int time;
} blinken_t;

#define BLINKEN_TIMER       LEDC_TIMER_0                  // Use first hardware timer
#define BLINKEN_MODE        LEDC_HIGH_SPEED_MODE          // Just use high speed (higher resolution)
#define BLINKEN_PWM_HZ      CONFIG_PWM_HZ                 // PWM frequency
#define BLINKEN_RESOLUTION  LEDC_TIMER_10_BIT             // PWM resolution
#define BLINKEN_MAX_DUTY    ((1 << 10) - 1)               // Maximum PWM value based on resolution
#define BLINKEN_MULTIPLIER  (BLINKEN_MAX_DUTY / CHAR_MAX) // For adjusting value range from 0-255
#define BLINKEN_MAP(x)      (x * BLINKEN_MAX_DUTY / CHAR_MAX)

#define BLINKEN_CH_NUM      (4)            // Number of LED channels (R,G,B,W)
#define BLINKEN_CHR_GPIO    CONFIG_R_GPIO  // GPIO output for red strip
#define BLINKEN_CHR_CHANNEL LEDC_CHANNEL_0 // LEDC channel for red strip
#define BLINKEN_CHG_GPIO    CONFIG_G_GPIO  // GPIO output for green strip
#define BLINKEN_CHG_CHANNEL LEDC_CHANNEL_1 // LEDC channel for green strip
#define BLINKEN_CHB_GPIO    CONFIG_B_GPIO  // GPIO output for blue strip
#define BLINKEN_CHB_CHANNEL LEDC_CHANNEL_2 // LEDC channel for blue strip
#define BLINKEN_CHW_GPIO    CONFIG_W_GPIO  // GPIO output for white strip
#define BLINKEN_CHW_CHANNEL LEDC_CHANNEL_3 // LEDC channel for white strip

void set_channel(value_t val, ledc_channel_t channel, int time) {
  uint32_t duty = val * BLINKEN_MAX_DUTY / VALUE_T_MAX;
  ESP_LOGI(TAG, "Setting channel %d to %d (%d duty over %dms)", channel, val, duty, time);
  ledc_set_fade_with_time(BLINKEN_MODE, channel, duty, time);
  ledc_fade_start(BLINKEN_MODE, channel, LEDC_FADE_NO_WAIT);
}


void set_channels(blinken_t *cfg) {
  // Red
  ESP_LOGI(TAG, "Setting red channel");
  set_channel(cfg->red, BLINKEN_CHR_CHANNEL, cfg->time);
  /*
  // Green
  ledc_set_fade_with_time(BLINKEN_MODE, BLINKEN_CHG_CHANNEL, config->green * 32, config->time);
  ledc_fade_start(BLINKEN_MODE, BLINKEN_CHG_CHANNEL, LEDC_FADE_NO_WAIT);
  // Blue
  ledc_set_fade_with_time(BLINKEN_MODE, BLINKEN_CHB_CHANNEL, config->blue * 32, config->time);
  ledc_fade_start(BLINKEN_MODE, BLINKEN_CHB_CHANNEL, LEDC_FADE_NO_WAIT);
  // White
  ledc_set_fade_with_time(BLINKEN_MODE, BLINKEN_CHW_CHANNEL, config->white * 32, config->time);
  ledc_fade_start(BLINKEN_MODE, BLINKEN_CHW_CHANNEL, LEDC_FADE_NO_WAIT);
  */
}

void app_main()
{
  int ch;

  // Set up LED timer for PWM control
  ledc_timer_config_t ledc_timer = {
    .duty_resolution = BLINKEN_RESOLUTION, // resolution of PWM duty
    .freq_hz = BLINKEN_PWM_HZ,             // frequency of PWM signal
    .speed_mode = BLINKEN_MODE,            // timer mode
    .timer_num = BLINKEN_TIMER             // timer index
  };
  ledc_timer_config(&ledc_timer);

  ledc_channel_config_t ledc_channel[BLINKEN_CH_NUM] = {
    {
      .channel    = BLINKEN_CHR_CHANNEL,
      .duty       = 0,
      .gpio_num   = BLINKEN_CHR_GPIO,
      .speed_mode = BLINKEN_MODE,
      .timer_sel  = BLINKEN_TIMER
    }/*,
       {
       .channel    = BLINKEN_CHG_CHANNEL,
       .duty       = 0,
       .gpio_num   = BLINKEN_CHG_GPIO,
       .speed_mode = BLINKEN_MODE,
       .timer_sel  = BLINKEN_TIMER
       },
       {
       .channel    = BLINKEN_CHB_CHANNEL,
       .duty       = 0,
       .gpio_num   = BLINKEN_CHB_GPIO,
       .speed_mode = BLINKEN_MODE,
       .timer_sel  = BLINKEN_TIMER
       },
       {
       .channel    = BLINKEN_CHW_CHANNEL,
       .duty       = 0,
       .gpio_num   = BLINKEN_CHW_GPIO,
       .speed_mode = BLINKEN_MODE,
       .timer_sel  = BLINKEN_TIMER
       },*/
  };

  // Set LED Controller with previously prepared configuration
  for (ch = 0; ch < BLINKEN_CH_NUM; ch++) {
    ledc_channel_config(&ledc_channel[ch]);
  }

  // Initialize fade service.
  ledc_fade_func_install(0);

  // Turn off LEDs
  blinken_t cfg;
  cfg.time = 3000;
  while(1) {
    cfg.red = 0;
    cfg.green = 0;
    cfg.blue = 0;
    cfg.white = 0;
      
    set_channels(&cfg);
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    cfg.red = 255;
    cfg.green = 255;
    cfg.blue = 255;
    cfg.white = 255;

    set_channels(&cfg);
    vTaskDelay(3000/ portTICK_PERIOD_MS);
  }
}
