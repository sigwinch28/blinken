#define _GNU_SOURCE
#include <stdio.h>

#include "parser.h"
#include "unity.h"

#include <stdint.h>

void test_assert_zero(blinken_t *b) {
  TEST_ASSERT(b->red == 0);
  TEST_ASSERT(b->green == 0);
  TEST_ASSERT(b->blue == 0);
  TEST_ASSERT(b->white == 0);
  TEST_ASSERT(b->time == 0);
}

/*
Init properly zeroes values  If memory allocation wasn't possible, or some other error occurs,
       these functions will return -1,
*/
void test_blinken_init() {
  blinken_t b;
  blinken_init(&b);
  test_assert_zero(&b);
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
Value parsing accepts the smallest value.
*/
void test_blinken_value_parse_min() {
  char *raw = "0";
  value_t val;
  char *res = value_parse(&val, raw);
  TEST_ASSERT_FALSE(res == raw);
  TEST_ASSERT(val == 0);
}

/*
Value parsing accepts the largest value.
*/
void test_blinken_value_parse_max() {
  char *raw = "255";
  value_t val;
  char *res = value_parse(&val, raw);
  TEST_ASSERT_FALSE(res == raw);
  TEST_ASSERT(val == VALUE_T_MAX);
}

void test_bliken_value_parse() {
  for (value_t val = 0; val <= VALUE_T_MAX; val++) {
    char *raw;
    TEST_ASSERT_FALSE(asprintf(&raw, "%3d", val) == -1);

    value_t val_res;
    char *res = value_parse(&val_res, raw);
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
  test_assert_zero(&b);
}

/*
Parsing single channel values
*/
void test_blinken_parse_single_channel() {
  blinken_t b;
  blinken_init(&b);
  char *raw = "R255";
  char* res = blinken_parse(&b, raw);
  TEST_ASSERT_FALSE(res == raw);
  TEST_ASSERT(b.red == UINT8_MAX);
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
  RUN_TEST(test_blinken_value_parse_min);
  RUN_TEST(test_blinken_value_parse_max);
  
  RUN_TEST(test_blinken_parse_null);
  RUN_TEST(test_blinken_parse_single_channel);
  
  return UNITY_END();
}
