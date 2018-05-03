#pragma once
#include <stddef.h>
#include <limits.h>
#include <stdint.h>

#define PARSER_BUF_LEN 32
#define PARSER_BUF_LEN_INT 8

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

void blinken_copy(blinken_t*, blinken_t*);

char *blinken_parse(blinken_t*, const char*);

int blinken_snprint(char**, size_t, blinken_t*);


