#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bproto.h"
#include "bproto_internal.h"

#include <check.h>
/*
#define TEST_TIME_LIMIT (255)

#define TEST_BPROTO_FMT "%c%d%c%d%c%d%c%d%c%d"
#define TEST_BPROTO_ASPRINTF(ptr, b) \
  TEST_ASSERT_FALSE(asprintf(ptr, TEST_BPROTO_FMT, \
    BPROTO_FIELD_RED,   b.red, \
    BPROTO_FIELD_GREEN, b.green, \
    BPROTO_FIELD_BLUE,  b.blue, \
    BPROTO_FIELD_WHITE, b.white, \
    BPROTO_FIELD_TIME,  b.time ) == -1)
*/
#define TEST_BPROTO_ASSERT_ZERO(b) \
  ck_assert(b.red   == 0); \
  ck_assert(b.green == 0); \
  ck_assert(b.blue  == 0); \
  ck_assert(b.white == 0); \
  ck_assert(b.time  == 0); \
  /*
#define TEST_BPROTO_ASSERT_EQ(x, y) \
  TEST_ASSERT(x.red   == y.red);   \
  TEST_ASSERT(x.green == y.green); \
  TEST_ASSERT(x.blue  == y.blue);  \
  TEST_ASSERT(x.white == y.white); \
  TEST_ASSERT(x.time  == y.time);

*/

#define TEST_ASSERT(x) ck_assert(x)
#define TEST_ASSERT_FALSE(x) TEST_ASSERT(!(x))

START_TEST(test_bproto_init)
{
  bproto_t b;
  bproto_init(&b);
  TEST_BPROTO_ASSERT_ZERO(b);
}
END_TEST

START_TEST(test_bproto_field_parse_null)
{

  char raw = '\0';
  bproto_field_t field;
  char *res = bproto_field_parse(&field, &raw);
  TEST_ASSERT(res == &raw);
}
END_TEST

START_TEST(test_bproto_field_parse)
{
  bproto_field_t chs[5] = {
    BPROTO_FIELD_RED,
    BPROTO_FIELD_GREEN,
    BPROTO_FIELD_BLUE,
    BPROTO_FIELD_WHITE,
    BPROTO_FIELD_TIME,
  };

  char raw[2];
  raw[1] = '\0';

  for (int i = 0; i < 5; i++) {
    raw[0] = (char) chs[i];
    
    bproto_field_t res_ch;
    char *res = bproto_field_parse(&res_ch, raw);

    TEST_ASSERT_FALSE(res == raw);
    TEST_ASSERT(res_ch == *raw);
    
  }
}
END_TEST


START_TEST(test_bproto_digit_parse_null)
{
  char raw = '\0';
  bproto_digit_t val;
  char *res = bproto_digit_parse(&val, &raw);
  TEST_ASSERT(res == &raw);
}
END_TEST


START_TEST(test_bproto_digit_parse) {
  for (int i = 0; i <= 9; i++) {
    char raw = '0' + i;
    bproto_digit_t val;
    char *res = bproto_digit_parse(&val, &raw);
    TEST_ASSERT_FALSE(res == &raw);
    TEST_ASSERT(val == i);
  }
}
END_TEST

/*
void test_bproto_value_parse_null() {
  char raw = '\0';
  bproto_value_t val;
  char *res = bproto_value_parse(&val, &raw);
  TEST_ASSERT(res == &raw);
}


void test_bproto_value_parse() {
  for (int val = 0; val <= BPROTO_VALUE_T_MAX; val++) {
    char *raw; 
    TEST_ASSERT_FALSE(asprintf(&raw, "%d", val) == -1);

    bproto_value_t val_res;
    char *res = bproto_value_parse(&val_res, raw);
    TEST_ASSERT_FALSE(res == raw);
    TEST_ASSERT(val_res == val);
  }
}


void test_bproto_value_parse_leading_zeroes() {
  for (int val = 0; val <= BPROTO_VALUE_T_MAX; val++) {
    char *raw; 
    TEST_ASSERT_FALSE(asprintf(&raw, "%03d", val) == -1);

    bproto_value_t val_res;
    char *res = bproto_value_parse(&val_res, raw);
    TEST_ASSERT_FALSE(res == raw);
    TEST_ASSERT(val_res == val);
  }
}

void test_bproto_fade_time_parse_null() {
  char raw = '\0';
  bproto_time_t val;
  char *res = bproto_time_parse(&val, &raw);
  TEST_ASSERT(res == &raw);
}


void test_bproto_fade_time_parse() {
  int cycles = 256;
  for (bproto_time_t val = 0; val < TEST_TIME_LIMIT; val++) {
    char *raw;
    TEST_ASSERT_FALSE(asprintf(&raw, "%d", val) == -1);

    bproto_time_t val_res;
    char *res = bproto_time_parse(&val_res, raw);
    TEST_ASSERT_FALSE(res == raw);
    TEST_ASSERT(val_res == val);
  }
}


void test_bproto_fade_time_parse_leading_zeroes() {
  int cycles = 256;
  for (bproto_time_t val = 0; val < TEST_TIME_LIMIT; val++) {
    char *raw; 
    TEST_ASSERT_FALSE(asprintf(&raw, "%03d", val) == -1);

    bproto_time_t val_res;
    char *res = bproto_time_parse(&val_res, raw);
    TEST_ASSERT_FALSE(res == raw);
    TEST_ASSERT(val_res == val);
  }
}

void test_bproto_parse_null() {
  bproto_t b;
  bproto_init(&b);
  char raw = '\0';
  char *ptr = &raw;
  char *res = bproto_parse(&b, ptr);
  TEST_ASSERT(res == ptr);
  TEST_BPROTO_ASSERT_ZERO(b);
}

void test_bproto_parse_all_channels_min() {
  bproto_t b, b_res;
  char *raw;
  char *res;
  
  bproto_init(&b);
  TEST_BPROTO_ASPRINTF(&raw, b);

  bproto_init(&b);
  res = bproto_parse(&b_res, raw);

  TEST_ASSERT_FALSE(res == raw);
  TEST_BPROTO_ASSERT_EQ(b, b_res);
}

void test_bproto_parse_all_channels_max() {
  bproto_t b, b_res;
  char *raw;
  char *res;
  
  bproto_init(&b);
  b.red =   BPROTO_VALUE_T_MAX;
  b.green = BPROTO_VALUE_T_MAX;
  b.blue =  BPROTO_VALUE_T_MAX;
  b.white = BPROTO_VALUE_T_MAX;
  b.time =  BPROTO_TIME_T_MAX;
  TEST_BPROTO_ASPRINTF(&raw, b);

  res = bproto_parse(&b_res, raw);

  TEST_ASSERT_FALSE(res == raw);
  TEST_BPROTO_ASSERT_EQ(b, b_res);
}

int main() {
  UNITY_BEGIN();
  
  RUN_TEST(test_bproto_init);

  // Field parsing
  RUN_TEST(test_bproto_field_parse_null);
  RUN_TEST(test_bproto_field_parse);

  // Digit Parsing
  RUN_TEST(test_bproto_digit_parse_null);
  RUN_TEST(test_bproto_digit_parse);

  // Value parsing
  RUN_TEST(test_bproto_value_parse_null);
  RUN_TEST(test_bproto_value_parse);
  RUN_TEST(test_bproto_value_parse_leading_zeroes);

  // Time parsing
  RUN_TEST(test_bproto_fade_time_parse_null);
  RUN_TEST(test_bproto_fade_time_parse);
  RUN_TEST(test_bproto_fade_time_parse_leading_zeroes);
  
  RUN_TEST(test_bproto_parse_null);
  RUN_TEST(test_bproto_parse_all_channels_min);
  RUN_TEST(test_bproto_parse_all_channels_max);

  bproto_t b;
  bproto_init(&b);
  char buf[128];
  char *res = buf;
  bproto_snprint(&res, 128, &b);

  return UNITY_END();
}*/

Suite *bproto_suite(void) {
  Suite *s = suite_create("bproto");
  TCase *tc_proto_t = tcase_create("bproto_t");

  tcase_add_test(tc_proto_t, test_bproto_init);
  suite_add_tcase(s, tc_proto_t);

  TCase *tc_field_parse = tcase_create("field_parse");

  tcase_add_test(tc_field_parse, test_bproto_field_parse_null);
  tcase_add_test(tc_field_parse, test_bproto_field_parse);
  suite_add_tcase(s, tc_field_parse);

  TCase *tc_digit_parse = tcase_create("digit_parse");

  tcase_add_test(tc_digit_parse, test_bproto_digit_parse_null);
  tcase_add_test(tc_digit_parse, test_bproto_digit_parse);
  suite_add_tcase(s, tc_digit_parse);

  return s;
}

int main(void) {
  int number_failed = 0;
  Suite *s;
  SRunner *sr;

  s = bproto_suite();
  sr = srunner_create(s);

  srunner_run_all(sr, CK_ENV);
  number_failed += srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
