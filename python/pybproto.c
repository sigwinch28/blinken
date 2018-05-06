#include <python2.7/Python.h>
#include <stdio.h>
#include "bproto.h"

#define PYBPROTO_MAX_LEN 32

#define PYBPROTO_KEY_RED   ("red")
#define PYBPROTO_KEY_GREEN ("green")
#define PYBPROTO_KEY_BLUE  ("blue")
#define PYBPROTO_KEY_WHITE ("white")
#define PYBPROTO_KEY_TIME  ("time")

static PyObject *PybprotoError;

static PyObject *pybproto_parse(PyObject*, PyObject*);
static PyObject *pybproto_new(PyObject*, PyObject*);
/*
static const char* pybproto_field_to_key(bproto_field_t f) {
  switch(f) {
  case BPROTO_FIELD_RED:   return PYBPROTO_KEY_RED;
  case BPROTO_FIELD_GREEN: return PYBPROTO_KEY_GREEN;
  case BPROTO_FIELD_BLUE:  return PYBPROTO_KEY_BLUE;
  case BPROTO_FIELD_WHITE: return PYBPROTO_KEY_WHITE;
  case BPROTO_FIELD_TIME:  return PYBPROTO_KEY_TIME;
  default:                 return NULL;
  }
}
*/
static PyMethodDef PybprotoMethods[] = {
  {"parse", pybproto_parse, METH_VARARGS,
   "Parse a bproto packet."},
  {"new", pybproto_new, METH_VARARGS,
   "Create a new bproto packet from a dict."},
  {NULL, NULL, 0, NULL} /* Sentinel */
};

PyMODINIT_FUNC
initpybproto(void) {
  PyObject *m;

  m = Py_InitModule("pybproto", PybprotoMethods);
  if (m == NULL) {
    return;
  }

  PybprotoError = PyErr_NewException("pybproto.error", NULL, NULL);
  Py_INCREF(PybprotoError);
  PyModule_AddObject(m, "error", PybprotoError);
}

static PyObject *bproto_to_pyobject(bproto_t *b) {
  return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i}",
		       PYBPROTO_KEY_RED,   b->red,
		       PYBPROTO_KEY_GREEN, b->green,
		       PYBPROTO_KEY_BLUE,  b->blue,
		       PYBPROTO_KEY_WHITE, b->white,
		       PYBPROTO_KEY_TIME,  b->time);
}

static PyObject *pybproto_parse(PyObject *self, PyObject *args) {
  bproto_t b;
  bproto_init(&b);

  char *raw;
  if (!PyArg_ParseTuple(args, "s!", &PyString_Type, &raw)) {
    return NULL;
  }

  char *res = bproto_parse(&b, raw);

  if (res == raw) {
    PyErr_SetString(PybprotoError, "Parse error");
    return NULL;
  }

  return bproto_to_pyobject(&b);
}

static int pybproto_key_to_long(PyObject *dict, char *key, long *dst) {
  PyObject *item = PyDict_GetItemString(dict, key);
  if (item == NULL) {
    *dst = 0;
    return 1;
  }

  *dst = PyInt_AsLong(item);
  if (*dst == -1 && PyErr_Occurred()) {
    char *err;
    if (asprintf(&err, "Invalid numeric value in '%s'", key) != -1) {
      PyErr_SetString(PyExc_ValueError, err);
    } else {
      PyErr_SetString(PyExc_ValueError, "Invalid numeric value");
    }
    free(err);
    return 0;
  }

  return 1;
}

static int pybproto_long_to_value_t(long src, bproto_value_t *dst) {
  if (src >= 0 && src <= BPROTO_VALUE_T_MAX) {
    *dst = (bproto_value_t) src;
    return 1;
  } else {
     char *err;
     if (asprintf(&err, "Value out of range: '%ld'", src) != -1) {
       PyErr_SetString(PyExc_ValueError, err);
     } else {
       PyErr_SetString(PyExc_ValueError, "Value out of range");
     }
     free(err);
     return 0;
  }
}

static int pybproto_long_to_time_t(long src, bproto_time_t *dst) {
  if (src >= 0 && src <= BPROTO_TIME_T_MAX) {
    *dst = (bproto_time_t) src;
    return 1;
  } else {
    char *err;
    if (asprintf(&err, "Value out of range: '%ld'", src) != -1) {
      PyErr_SetString(PyExc_ValueError, err);
    } else {
      PyErr_SetString(PyExc_ValueError, "Value out of range");
    }
    free(err);
    return 0;
  }
}

#define RETURN_IF_FALSE(x)      \
  do {			       \
    if (!x) {		       \
      return NULL;	       \
    }			       \
  } while(0);

static PyObject *pybproto_new(PyObject *self, PyObject *args) {
  char buf[PYBPROTO_MAX_LEN];


  PyObject* dict;
  PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict);

  bproto_t b;
  bproto_init(&b);
  long res;

  RETURN_IF_FALSE(pybproto_key_to_long(dict, PYBPROTO_KEY_RED, &res));
  RETURN_IF_FALSE(pybproto_long_to_value_t(res, &b.red));

  RETURN_IF_FALSE(pybproto_key_to_long(dict, PYBPROTO_KEY_GREEN, &res));
  RETURN_IF_FALSE(pybproto_long_to_value_t(res, &b.green));

  RETURN_IF_FALSE(pybproto_key_to_long(dict, PYBPROTO_KEY_BLUE, &res));
  RETURN_IF_FALSE(pybproto_long_to_value_t(res, &b.blue));

  RETURN_IF_FALSE(pybproto_key_to_long(dict, PYBPROTO_KEY_WHITE, &res));
  RETURN_IF_FALSE(pybproto_long_to_value_t(res, &b.white));

  
  RETURN_IF_FALSE(pybproto_key_to_long(dict, PYBPROTO_KEY_TIME, &res));
  RETURN_IF_FALSE(pybproto_long_to_time_t(res, &b.time));

  char *ptr = buf;
  
  if(bproto_snprint(&ptr, PYBPROTO_MAX_LEN, &b)) {
    return Py_BuildValue("s", buf);
  } else {
    PyErr_SetString(PyExc_ValueError, "Internal buffer too small");
    return NULL;
  }
}
