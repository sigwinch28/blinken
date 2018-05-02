#include "parser.h"

void blinken_init(blinken_t *b) {
  b->red = 0;
  b->green = 0;
  b->blue = 0;
  b->white = 0;
  b->time = 0;
}

/*
y is target
*/
void blinken_copy(blinken_t *x, blinken_t *y) {
  y->red = x->red;
  y->green = x->green;
  y->blue = x->blue;
  y->white = x->white;
  y->time = x->time;
}

char *field_parse(field_t *cmd, const char *ptr) {
  switch (*ptr) {
  case FIELD_RED:
  case FIELD_GREEN:
  case FIELD_BLUE:
  case FIELD_WHITE:
  case FIELD_TIME:
    *cmd = *(ptr++);
    return (char *) ptr;
  default:
    return (char *)ptr;
  }
}

char *digit_parse(char *val, const char *ptr) {
  if (*ptr >= '0' && *ptr <= '9') {
    *val = *(ptr++) - '0';
  }
  return (char *)ptr;
}

char *value_parse(value_t *val, const char *ptr) {
  char *orig = (char *)ptr;
  for (*val = 0;; ptr++) {
    char digit;
    char *res = digit_parse(&digit, ptr);
    if (res == ptr) {
      return (char*) ptr;
    }

    if (((*val) * 10) + digit <= VALUE_T_MAX) {
      *val *= 10;
      *val += digit;
    } else {
      *val = 0;
      return orig;
    }
  }
}

char *fade_time_parse(fade_time_t *val, const char *ptr) {
  char *orig = (char *)ptr;
  for (*val = 0;; ptr++) {
    char digit;
    char *res = digit_parse(&digit, ptr);
    if (res == ptr) {
      return (char*) ptr;
    }

    if (((*val) * 10) + digit <= FADE_TIME_T_MAX) {
      *val *= 10;
      *val += digit;
    } else {
      *val = 0;
      return orig;
    }
  }
}



char *blinken_parse(blinken_t *cfg, const char *ptr) {
  blinken_init(cfg);
  char *res;
  char *orig = (char *)ptr;

  while (*ptr != '\0') {
    field_t field;

    res = field_parse(&field, ptr);
    if (res == ptr) {
      return orig;
    }
    ptr = res;

    switch (field) {
    case FIELD_RED:
      res = value_parse(&(cfg->red), ptr);
      break;
    case FIELD_GREEN:
      res = value_parse(&(cfg->green), ptr);
      break;
    case FIELD_BLUE:
      res = value_parse(&(cfg->blue), ptr);
      break;
    case FIELD_WHITE:
      res = value_parse(&(cfg->white), ptr);
      break;
    case FIELD_TIME:
      res = fade_time_parse(&(cfg->time), ptr);
      break;
    }

    if (res == ptr) {
      return orig;
    }
    ptr = res;
  }

  return (char *)ptr;
}
