#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "unity.h"

#include <stdint.h>

#define TEST_TIME_LIMIT (255)

#define TEST_BLINKEN_FMT "%c%d%c%d%c%d%c%d%c%d"
#define TEST_BLINKEN_ASPRINTF(ptr, b) \
  TEST_ASSERT_FALSE(asprintf(ptr, TEST_BLINKEN_FMT, \
    FIELD_RED,   b.red, \
    FIELD_GREEN, b.green, \
    FIELD_BLUE,  b.blue, \
    FIELD_WHITE, b.white, \
    FIELD_TIME,  b.time ) == -1)

#define TEST_BLINKEN_ASSERT_ZERO(b) \
  TEST_ASSERT(b.red   == 0); \
  TEST_ASSERT(b.green == 0); \
  TEST_ASSERT(b.blue  == 0); \
  TEST_ASSERT(b.white == 0); \
  TEST_ASSERT(b.time  == 0); \

#define TEST_BLINKEN_ASSERT_EQ(x, y) \
  TEST_ASSERT(x.red   == y.red);   \
  TEST_ASSERT(x.green == y.green); \
  TEST_ASSERT(x.blue  == y.blue);  \
  TEST_ASSERT(x.white == y.white); \
  TEST_ASSERT(x.time  == y.time);

/*
Init properly zeroes values
*/
void test_blinken_init() {
  blinken_t b;
  blinken_init(&b);
  TEST_BLINKEN_ASSERT_ZERO(b);
}

/*
Field parser ignores null values.
*/
void test_blinken_field_parse_null() {
  char raw = '\0';
  field_t field;
  char *res = field_parse(&field, &raw);
  TEST_ASSERT(res == &raw);
}

/*
Field parser correctly parses all fields from enum.
*/
void test_blinken_field_parse() {
  field_t chs[5] = {
    FIELD_RED,
    FIELD_GREEN,
    FIELD_BLUE,
    FIELD_WHITE,
    FIELD_TIME,
  };

  char raw[2];
  raw[1] = '\0';

  for (int i = 0; i < 5; i++) {
    raw[0] = (char) chs[i];
    
    field_t res_ch;
    char *res = field_parse(&res_ch, raw);

    TEST_ASSERT_FALSE(res == raw);
    TEST_ASSERT(res_ch == *raw);
    
  }
}

/*
Digit parsing ignores null values.
*/
void test_blinken_digit_parse_null() {
  char raw = '\0';
  char val;
  char *res = digit_parse(&val, &raw);
  TEST_ASSERT(res == &raw);
}

/*
All single digits parse correctly.
*/
void test_blinken_digit_parse() {
  for (int i = 0; i <= 9; i++) {
    char raw = '0' + i;
    char val;
    char *res = digit_parse(&val, &raw);
    TEST_ASSERT_FALSE(res == &raw);
    TEST_ASSERT(val == i);
  }
}

/*
Value parsing ignores null values.
*/
void test_blinken_value_parse_null() {
  char raw = '\0';
  value_t val;
  char *res = value_parse(&val, &raw);
  TEST_ASSERT(res == &raw);
}

/*
Value parsing accepts all values in range.
*/
void test_blinken_value_parse() {
  for (int val = 0; val <= VALUE_T_MAX; val++) {
    char *raw; 
    TEST_ASSERT_FALSE(asprintf(&raw, "%d", val) == -1);

    value_t val_res;
    char *res = value_parse(&val_res, raw);
    TEST_ASSERT_FALSE(res == raw);
    TEST_ASSERT(val_res == val);
  }
}

/*
Value parsing accepts all values in range.
*/
void test_blinken_value_parse_leading_zeroes() {
  for (int val = 0; val <= VALUE_T_MAX; val++) {
    char *raw; 
    TEST_ASSERT_FALSE(asprintf(&raw, "%03d", val) == -1);

    value_t val_res;
    char *res = value_parse(&val_res, raw);
    TEST_ASSERT_FALSE(res == raw);
    TEST_ASSERT(val_res == val);
  }
}

/*
Time parsing ignores null values.
*/
void test_blinken_fade_time_parse_null() {
  char raw = '\0';
  fade_time_t val;
  char *res = fade_time_parse(&val, &raw);
  TEST_ASSERT(res == &raw);
}

/*
Time parsing accepts all values in range.
*/
void test_blinken_fade_time_parse() {
  int cycles = 256;
  for (fade_time_t val = 0; val < TEST_TIME_LIMIT; val++) {
    char *raw;
    TEST_ASSERT_FALSE(asprintf(&raw, "%d", val) == -1);

    fade_time_t val_res;
    char *res = fade_time_parse(&val_res, raw);
    TEST_ASSERT_FALSE(res == raw);
    TEST_ASSERT(val_res == val);
  }
}

/*
Time parsing accepts all values in range.
*/
void test_blinken_fade_time_parse_leading_zeroes() {
  int cycles = 256;
  for (fade_time_t val = 0; val < TEST_TIME_LIMIT; val++) {
    char *raw; 
    TEST_ASSERT_FALSE(asprintf(&raw, "%03d", val) == -1);

    fade_time_t val_res;
    char *res = fade_time_parse(&val_res, raw);
    TEST_ASSERT_FALSE(res == raw);
    TEST_ASSERT(val_res == val);
  }
}

/*
Parsing a null value changes nothing
*/
void test_blinken_parse_null() {
  blinken_t b;
  blinken_init(&b);
  char raw = '\0';
  char *ptr = &raw;
  char *res = blinken_parse(&b, ptr);
  TEST_ASSERT(res == ptr);
  TEST_BLINKEN_ASSERT_ZERO(b);
}

/*
Parsing zero values is correct
*/
void test_blinken_parse_all_channels_min() {
  blinken_t b, b_res;
  char *raw;
  char *res;
  
  blinken_init(&b);
  TEST_BLINKEN_ASPRINTF(&raw, b);

  blinken_init(&b);
  res = blinken_parse(&b_res, raw);

  TEST_ASSERT_FALSE(res == raw);
  TEST_BLINKEN_ASSERT_EQ(b, b_res);
}

/*
Parsing max values is correct
*/
void test_blinken_parse_all_channels_max() {
  blinken_t b, b_res;
  char *raw;
  char *res;
  
  blinken_init(&b);
  b.red = VALUE_T_MAX;
  b.green = VALUE_T_MAX;
  b.blue = VALUE_T_MAX;
  b.white = VALUE_T_MAX;
  b.time = FADE_TIME_T_MAX;
  TEST_BLINKEN_ASPRINTF(&raw, b);

  res = blinken_parse(&b_res, raw);

  TEST_ASSERT_FALSE(res == raw);
  TEST_BLINKEN_ASSERT_EQ(b, b_res);
}

int main() {
  UNITY_BEGIN();
  
  RUN_TEST(test_blinken_init);

  // Field parsing
  RUN_TEST(test_blinken_field_parse_null);
  RUN_TEST(test_blinken_field_parse);

  // Digit Parsing
  RUN_TEST(test_blinken_digit_parse_null);
  RUN_TEST(test_blinken_digit_parse);

  // Value parsing
  RUN_TEST(test_blinken_value_parse_null);
  RUN_TEST(test_blinken_value_parse);
  RUN_TEST(test_blinken_value_parse_leading_zeroes);

  // Time parsing
  RUN_TEST(test_blinken_fade_time_parse_null);
  RUN_TEST(test_blinken_fade_time_parse);
  RUN_TEST(test_blinken_fade_time_parse_leading_zeroes);
  
  RUN_TEST(test_blinken_parse_null);
  RUN_TEST(test_blinken_parse_all_channels_min);
  RUN_TEST(test_blinken_parse_all_channels_max);

  return UNITY_END();
}
