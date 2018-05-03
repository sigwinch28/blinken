#include "parser.h"

static char *field_parse(field_t*, const char*);

static int field_snprint(char**, size_t, field_t);

static char *digit_parse(char*, const char*);

static char *value_parse(value_t*, const char*);

static int value_snprint(char**, size_t, value_t);

static char *fade_time_parse(fade_time_t*, const char*);

static int fade_time_snprint(char**, size_t, fade_time_t);

static int int_snprint(char**, size_t, int);

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


char *blinken_parse(blinken_t *cfg, const char *ptr) {
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

#define ADD_OR_RETURN(res) \
  do { \
  int res2 = res; \
  if (res2 == 0) { \
  return 0; \
  } else { \
  i += res2; \
  } \
  } while(0)

int blinken_snprint(char **str, size_t size, blinken_t *b) {
  int i = 0;

  ADD_OR_RETURN(field_snprint(str, size-i, FIELD_RED));
  ADD_OR_RETURN(value_snprint(str, size-i, b->red));

  ADD_OR_RETURN(field_snprint(str, size-i, FIELD_GREEN));
  ADD_OR_RETURN(value_snprint(str, size-i, b->green));

  ADD_OR_RETURN(field_snprint(str, size-i, FIELD_BLUE));
  ADD_OR_RETURN(value_snprint(str, size-i, b->blue));

  ADD_OR_RETURN(field_snprint(str, size-i, FIELD_WHITE));
  ADD_OR_RETURN(value_snprint(str, size-i, b->white));

  ADD_OR_RETURN(field_snprint(str, size-i, FIELD_TIME));
  ADD_OR_RETURN(fade_time_snprint(str, size-i, b->time));
  
  if (i < size) {
    **str = '\0';
    *str+=1;
    return i+1;
  } else {
    return 0;
  }

}

static char *field_parse(field_t *cmd, const char *ptr) {
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

static int field_snprint(char **str, size_t size, field_t field) {
  if (size > 0) {
    **str = field;
    *str = *str+1;
    return 1;
  } else {
    return 0;
  }
}

static char *digit_parse(char *val, const char *ptr) {
  if (*ptr >= '0' && *ptr <= '9') {
    *val = *(ptr++) - '0';
  }
  return (char *)ptr;
}

static char *value_parse(value_t *val, const char *ptr) {
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

static int value_snprint(char **str, size_t size, value_t val) {
  return int_snprint(str, size, val);
}

static char *fade_time_parse(fade_time_t *val, const char *ptr) {
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

static int fade_time_snprint(char **str, size_t size, fade_time_t time) {
  return int_snprint(str, size, time);
}


static int int_snprint(char **ptr, size_t size, int val) {
  char *str = *ptr;
  char buf[PARSER_BUF_LEN_INT];
  int i = 0;

  while(i < PARSER_BUF_LEN) {
    buf[i++] = (val % 10) + '0';
    val /= 10;

    if (val <= 0) {
      break;
    }
  }

  if (i >= PARSER_BUF_LEN || size < i) {
    return 0;
  }

  int new_size = i--;

  while(i >= 0) {
    *(str++) = buf[i--];
  }
  *ptr = str;
  return new_size;
}
