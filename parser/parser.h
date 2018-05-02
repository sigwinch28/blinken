#pragma once
#include <limits.h>
#include <stdint.h>

typedef uint8_t value_t;
#define VALUE_T_MAX UINT8_MAX

typedef int fade_time_t;
#define FADE_TIME_T_MAX INT_MAX

typedef struct {
  value_t red, green, blue, white;
  fade_time_t time;
} blinken_t;

typedef enum {
  FIELD_RED = 'R',
  FIELD_GREEN = 'G',
  FIELD_BLUE = 'B',
  FIELD_WHITE = 'W',
  FIELD_TIME = 'T',
} field_t;

void blinken_init(blinken_t*);

char *field_parse(field_t*, const char*);

char *digit_parse(char*, const char*);

char *value_parse(value_t*, const char*);

char *fade_time_parse(fade_time_t*, const char*);

char *blinken_parse(blinken_t*, const char*);
