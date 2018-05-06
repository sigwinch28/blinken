#include <stdio.h>
#include "bproto.h"
#include "bproto_internal.h"

void bproto_init(bproto_t *b) {
  b->red = BPROTO_VALUE_UNSET;
  b->green = BPROTO_VALUE_UNSET;
  b->blue = BPROTO_VALUE_UNSET;
  b->white = BPROTO_VALUE_UNSET;
  b->time = BPROTO_TIME_UNSET;
}

/*
y is target
*/
void bproto_copy(bproto_t *x, bproto_t *y) {
  y->red = x->red;
  y->green = x->green;
  y->blue = x->blue;
  y->white = x->white;
  y->time = x->time;
}

int bproto_eq(bproto_t *x, bproto_t *y) {
  return
    x->red   == y->red   &&
    x->green == y->green &&
    x->blue  == y->blue  &&
    x->white == y->white &&
    x->time  == y->time;
}

int bproto_is_set(bproto_t *b) {
  bproto_t init;
  bproto_init(&init);
  return !bproto_eq(b, &init);
}

char *bproto_parse(bproto_t *cfg, const char *ptr) {
  char *res;
  char *orig = (char *)ptr;
  bproto_init(cfg);

  while (*ptr != '\0') {
    bproto_field_t field;

    res = bproto_field_parse(&field, ptr);
    if (res == ptr) {
      return orig;
    }
    ptr = res;

    switch (field) {
    case BPROTO_FIELD_RED:
      res = bproto_value_parse(&(cfg->red), ptr);
      break;
    case BPROTO_FIELD_GREEN:
      res = bproto_value_parse(&(cfg->green), ptr);
      break;
    case BPROTO_FIELD_BLUE:
      res = bproto_value_parse(&(cfg->blue), ptr);
      break;
    case BPROTO_FIELD_WHITE:
      res = bproto_value_parse(&(cfg->white), ptr);
      break;
    case BPROTO_FIELD_TIME:
      res = bproto_time_parse(&(cfg->time), ptr);
      break;
    }

    if (res == ptr) {
      return orig;
    }
    ptr = res;
  }

  return (char *)ptr;
}

#define ADD_OR_RETURN(res) \
  do { \
  int res2 = res; \
  if (res2 == 0) { \
  return 0; \
  } else { \
  i += res2; \
  } \
  } while(0)

int bproto_snprint(char **str, size_t size, bproto_t *b) {
  int i = 0;

  if (b->red != BPROTO_VALUE_UNSET) {
    ADD_OR_RETURN(bproto_field_snprint(str, size-i, BPROTO_FIELD_RED));
    ADD_OR_RETURN(bproto_value_snprint(str, size-i, b->red));
  }

  if (b->green != BPROTO_VALUE_UNSET) {
    ADD_OR_RETURN(bproto_field_snprint(str, size-i, BPROTO_FIELD_GREEN));
    ADD_OR_RETURN(bproto_value_snprint(str, size-i, b->green));
  }

  if (b->blue != BPROTO_VALUE_UNSET) {
    ADD_OR_RETURN(bproto_field_snprint(str, size-i, BPROTO_FIELD_BLUE));
    ADD_OR_RETURN(bproto_value_snprint(str, size-i, b->blue));
  }

  if (b->white != BPROTO_VALUE_UNSET) {
    ADD_OR_RETURN(bproto_field_snprint(str, size-i, BPROTO_FIELD_WHITE));
    ADD_OR_RETURN(bproto_value_snprint(str, size-i, b->white));
  }

  if (b->time != BPROTO_TIME_UNSET) {
    ADD_OR_RETURN(bproto_field_snprint(str, size-i, BPROTO_FIELD_TIME));
    ADD_OR_RETURN(bproto_time_snprint(str, size-i, b->time));
  }

  return i;
  /*
  if (i < size) {
    **str = '\0';
    *str+=1;
    return i+1;
  } else {
    return 0;
  }
  */
}

char *bproto_field_parse(bproto_field_t *cmd, const char *ptr) {
  switch (*ptr) {
  case BPROTO_FIELD_RED:
  case BPROTO_FIELD_GREEN:
  case BPROTO_FIELD_BLUE:
  case BPROTO_FIELD_WHITE:
  case BPROTO_FIELD_TIME:
    *cmd = *(ptr++);
    return (char *) ptr;
  default:
    return (char *)ptr;
  }
}

int bproto_field_snprint(char **str, size_t size, bproto_field_t field) {
  if (size > 0) {
    **str = field;
    *str = *str+1;
    return 1;
  } else {
    return 0;
  }
}

char *bproto_digit_parse(bproto_digit_t *val, const char *ptr) {
  if (*ptr >= '0' && *ptr <= '9') {
    *val = *(ptr++) - '0';
  }
  return (char *)ptr;
}

char *bproto_value_parse(bproto_value_t *val, const char *ptr) {
  char *orig = (char *)ptr;
  for (*val = 0;; ptr++) {
    bproto_digit_t digit;
    char *res = bproto_digit_parse(&digit, ptr);
    if (res == ptr) {
      return (char*) ptr;
    }

    if (((*val) * 10) + digit <= BPROTO_VALUE_T_MAX) {
      *val *= 10;
      *val += digit;
    } else {
      *val = 0;
      return orig;
    }
  }
}

int bproto_value_snprint(char **str, size_t size, bproto_value_t val) {
  if (val < BPROTO_VALUE_T_MIN || val > BPROTO_VALUE_T_MAX) {
    return 0;
  }
  return bproto_int_snprint(str, size, val);
}

char *bproto_time_parse(bproto_time_t *val, const char *ptr) {
  char *orig = (char *)ptr;
  for (*val = 0;; ptr++) {
    bproto_digit_t digit;
    char *res = bproto_digit_parse(&digit, ptr);
    if (res == ptr) {
      return (char*) ptr;
    }

    if (((*val) * 10) + digit <= BPROTO_TIME_T_MAX) {
      *val *= 10;
      *val += digit;
    } else {
      *val = 0;
      return orig;
    }
  }
}

int bproto_time_snprint(char **str, size_t size, bproto_time_t time) {
  if (time < BPROTO_TIME_T_MIN || time > BPROTO_TIME_T_MAX) {
    return 0;
  }
  return bproto_int_snprint(str, size, time);
}

int bproto_int_snprint(char **ptr, size_t size, int val) {
  char *str = *ptr;
  char buf[BPROTO_BUF_LEN_INT];
  int i = 0;

  while(i < BPROTO_BUF_LEN_INT) {
    buf[i++] = (val % 10) + '0';
    val /= 10;

    if (val <= 0) {
      break;
    }
  }

  if (i >= BPROTO_BUF_LEN_INT || size < i) {
    return 0;
  }

  int new_size = i--;

  while(i >= 0) {
    *(str++) = buf[i--];
  }
  *ptr = str;
  return new_size;
}
