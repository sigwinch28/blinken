#pragma once

#define BLINKEN_TIMER LEDC_TIMER_0 // Use first hardware timer
#define BLINKEN_MODE                                                           \
  LEDC_HIGH_SPEED_MODE               // Just use high speed (higher resolution)
#define BLINKEN_PWM_HZ CONFIG_PWM_HZ // PWM frequency
#define BLINKEN_RESOLUTION LEDC_TIMER_10_BIT // PWM resolution
#define BLINKEN_MAX_DUTY                                                       \
  ((1 << 10) - 1) // Maximum PWM value based on resolution
#define BLINKEN_MULTIPLIER                                                     \
  (BLINKEN_MAX_DUTY / CHAR_MAX) // For adjusting value range from 0-255
#define BLINKEN_MAP(x) (x * BLINKEN_MAX_DUTY / CHAR_MAX)

#define BLINKEN_CH_NUM (4)                 // Number of LED channels (R,G,B,W)
#define BLINKEN_CHR_GPIO CONFIG_R_GPIO     // GPIO output for red strip
#define BLINKEN_CHR_CHANNEL LEDC_CHANNEL_0 // LEDC channel for red strip
#define BLINKEN_CHG_GPIO CONFIG_G_GPIO     // GPIO output for green strip
#define BLINKEN_CHG_CHANNEL LEDC_CHANNEL_1 // LEDC channel for green strip
#define BLINKEN_CHB_GPIO CONFIG_B_GPIO     // GPIO output for blue strip
#define BLINKEN_CHB_CHANNEL LEDC_CHANNEL_2 // LEDC channel for blue strip
#define BLINKEN_CHW_GPIO CONFIG_W_GPIO     // GPIO output for white strip
#define BLINKEN_CHW_CHANNEL LEDC_CHANNEL_3 // LEDC channel for white strip
