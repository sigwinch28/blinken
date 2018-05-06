#pragma once
#include <stddef.h>
#include <limits.h>
#include <stdint.h>

typedef int16_t bproto_value_t;
#define BPROTO_VALUE_T_MIN (0)
#define BPROTO_VALUE_T_MAX (255)
#define BPROTO_VALUE_UNSET (-1)

typedef int32_t bproto_time_t;
#define BPROTO_TIME_T_MIN (0)
#define BPROTO_TIME_T_MAX (2147483647)
#define BPROTO_TIME_UNSET (-1)

typedef uint8_t bproto_digit_t;

#define BPROTO_BUF_LEN_INT 16

typedef struct {
  bproto_value_t red, green, blue, white;
  bproto_time_t time;
} bproto_t;

typedef enum {
  BPROTO_FIELD_RED = 'R',
  BPROTO_FIELD_GREEN = 'G',
  BPROTO_FIELD_BLUE = 'B',
  BPROTO_FIELD_WHITE = 'W',
  BPROTO_FIELD_TIME = 'T',
} bproto_field_t;

void bproto_init(bproto_t*);

void bproto_copy(bproto_t*, bproto_t*);

int bproto_eq(bproto_t*, bproto_t*);

int bproto_is_set(bproto_t*);

char *bproto_parse(bproto_t*, const char*);

int bproto_snprint(char**, size_t, bproto_t*);


