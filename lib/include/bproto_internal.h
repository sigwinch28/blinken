#include "bproto.h"

char *bproto_field_parse(bproto_field_t*, const char*);

int bproto_field_snprint(char**, size_t, bproto_field_t);

char *bproto_value_parse(bproto_value_t*, const char*);

int bproto_value_snprint(char**, size_t, bproto_value_t);

char *bproto_time_parse(bproto_time_t*, const char*);

int bproto_time_snprint(char**, size_t, bproto_time_t);

int bproto_int_snprint(char**, size_t, int);

char *bproto_digit_parse(bproto_digit_t*, const char*);
