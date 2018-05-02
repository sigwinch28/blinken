#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "blinken";
#include "esp_log.h"

#include "blinken_main.h"

static inline void set_channel(value_t val, ledc_channel_t channel, int time) {
  uint32_t duty = val * BLINKEN_MAX_DUTY / VALUE_T_MAX;
  ESP_LOGI(TAG, "Setting channel %d to %d (%d duty over %dms)", channel, val,
           duty, time);
  ledc_set_fade_with_time(BLINKEN_MODE, channel, duty, time);
  ledc_fade_start(BLINKEN_MODE, channel, LEDC_FADE_NO_WAIT);
}

void set_channels(blinken_t *cfg) {
  ESP_LOGI(TAG, "Setting all channels");
  set_channel(cfg->red, BLINKEN_CHR_CHANNEL, cfg->time);
  set_channel(cfg->green, BLINKEN_CHG_CHANNEL, cfg->time);
  set_channel(cfg->blue, BLINKEN_CHB_CHANNEL, cfg->time);
  set_channel(cfg->white, BLINKEN_CHW_CHANNEL, cfg->time);
}

void app_main() {
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

  // Set LED Controller with previously prepared configuration
  for (ch = 0; ch < BLINKEN_CH_NUM; ch++) {
    ledc_channel_config(&ledc_channel[ch]);
  }

  // Initialize fade service.
  ledc_fade_func_install(0);

  // Turn off LEDs
  blinken_t cfg;
  cfg.time = 3000;
  while (1) {
    cfg.red = 0;
    cfg.green = 0;
    cfg.blue = 0;
    cfg.white = 0;

    set_channels(&cfg);
    vTaskDelay(6000 / portTICK_PERIOD_MS);

    cfg.red = VALUE_T_MAX;
    cfg.green = VALUE_T_MAX;
    cfg.blue = VALUE_T_MAX;
    cfg.white = VALUE_T_MAX;

    set_channels(&cfg);
    vTaskDelay(6000 / portTICK_PERIOD_MS);
  }
}
