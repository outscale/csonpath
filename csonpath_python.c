#include <Python.h>

#define CSONPATH_JSON PyObject *

#define CSONPATH_NULL Py_None

#define CSONPATH_GET csonpath_python_get

#define CSONPATH_AT csonpath_python_at

#define CSONPATH_REMOVE(o) // PyList_Clear(o)

#define CSONPATH_NEW_ARRAY() PyList_New(0)

#define CSONPATH_NEW_INT(i)			\
  PyLong_FromLong(i)

#define CSONPATH_NEW_STR(key)			\
  PyUnicode_FromString(key)

#define CSONPATH_IS_OBJ(o) PyDict_Check(o)

#define CSONPATH_IS_ARRAY(o) PyList_Check(o)


/* assuming each modification of the object need to go out of the loop */
#define CSONPATH_NEED_FOREACH_REDO(o)	1

#define CSONPATH_REMOVE_CHILD(obj, child_info)				\
  if (child_info.type == CSONPATH_INTEGER) {				\
    PyList_SetSlice(obj, child_info.idx, child_info.idx + 1, NULL);	\
  } else if (child_info.type == CSONPATH_STR) {				\
    PyDict_DelItemString(obj, child_info.key);				\
  }

#define CSONPATH_ARRAY_APPEND(list, el) PyList_Append(list, el)

/* I don't think incrref is needed with python */
#define CSONPATH_ARRAY_APPEND_INCREF(array, el) ({	\
      PyList_Append(array, el);				\
      Py_INCREF(el);					\
    })

#define CSONPATH_FOREACH(obj, el, code)					\
  if (PyDict_Check(obj)) {						\
    PyObject *key_;							\
    Py_ssize_t pos_ = 0;						\
    while (PyDict_Next(obj, &pos_, &key_, &el)) {			\
      const char *key_idx = PyUnicode_AsUTF8AndSize(key_, NULL);	\
      (void)key_idx;							\
      code								\
	}								\
  } else if (PyList_Check(obj)) {					\
    int array_len_ = PyList_Size(obj);					\
    for (intptr_t key_idx = 0; key_idx < array_len_; ++key_idx) {		\
      el = PyList_GetItem(obj, key_idx);				\
      code								\
	}								\
  }

static PyObject *csonpath_python_get(PyObject *obj, const char *key) {
    if (PyDict_Check(obj)) {
        PyObject *value = PyDict_GetItemString(obj, key);
        return value ? value : Py_None;
    }
    return Py_None;
}

static PyObject *csonpath_python_at(PyObject *obj, int at) {
    if (PyList_Check(obj)) {
        if (at >= 0 && at < PyList_Size(obj)) {
            PyObject *item = PyList_GetItem(obj, at);
            return item ? item : Py_None;
        }
    }
    return Py_None;
}

#include "csonpath.h"

typedef struct {
    PyObject_HEAD
    struct csonpath *cp;
} PyCsonPathObject;

#define CAPSULE_NAME "csonpath_capsule"

static PyObject *PyCsonPath_new(PyTypeObject *subtype, PyObject* args,
				PyObject* dont_care)
{
  PyCsonPathObject *self = (PyCsonPathObject *)subtype->tp_alloc(subtype, 0);
    const char *s;

    if (!self)
      return Py_None;

    if (!PyArg_ParseTuple(args, "s", &s))
        return Py_None;

    struct csonpath *ret = malloc(sizeof *ret);
    if (!self)
      return Py_None;

    if (csonpath_init(ret, s) < 0)
      return Py_None;

    if (csonpath_compile(ret) < 0)
      return Py_None;

    self->cp = ret;

    return (PyObject *)self;
}

static PyObject *find_all(PyCsonPathObject *self, PyObject* args)
{
  PyObject *json;

  if (!PyArg_ParseTuple(args, "O", &json))
    return Py_None;
  PyObject *ret =  csonpath_find_all(self->cp, json);
  if (ret != Py_None) {
      Py_INCREF(ret);
  }
  return ret;
}

static PyObject *find_first(PyCsonPathObject *self, PyObject* args)
{
  PyObject *json;

  if (!PyArg_ParseTuple(args, "O", &json))
    return Py_None;
  PyObject *ret = csonpath_find_first(self->cp, json);
  return ret;
}

static PyObject *do_remove(PyCsonPathObject *self, PyObject* args)
{
  PyObject *json;

  if (!PyArg_ParseTuple(args, "O", &json))
    return Py_None;
  int ret = csonpath_remove(self->cp, json);
  return PyLong_FromLong(ret);
}

static void PyCsonPath_dealloc(PyCsonPathObject *self) {
  if (self->cp) {
    csonpath_destroy(self->cp);
    free(self->cp);
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject *PyCsonPath_get_path(PyCsonPathObject *self, void *closure) {
    return PyUnicode_FromString(self->cp->path);
}

static int PyCsonPath_set_path(PyCsonPathObject *self, PyObject *value, void *closure) {
    const char *new_path = PyUnicode_AsUTF8(value);
    if (!new_path) return -1;

    csonpath_set_path(self->cp, new_path);
    return 0;
}

static PyMethodDef csonpath_py_method[] = {
    {"find_first", (PyCFunction)find_first, METH_VARARGS, "find first elems"},
    {"find_all", (PyCFunction)find_all, METH_VARARGS, "find all elems, if one found, pout it in an array"},
    {"remove", (PyCFunction)do_remove, METH_VARARGS, "remove all elems found"},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject PyCsonPathType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "csonpath.CsonPath",
    .tp_basicsize = sizeof(PyCsonPathObject),
    .tp_dealloc = (destructor)PyCsonPath_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = csonpath_py_method,
    .tp_new = PyCsonPath_new
};

static struct PyModuleDef csonpath_py_mod = {
    PyModuleDef_HEAD_INIT,
    "csonpath",
    NULL,
    -1,
    NULL
};

PyMODINIT_FUNC PyInit_csonpath(void) {
  PyObject *m;
  if (PyType_Ready(&PyCsonPathType) < 0)
    return NULL;

  m = PyModule_Create(&csonpath_py_mod);
  if (!m) return NULL;

  Py_INCREF(&PyCsonPathType);
  PyModule_AddObject(m, "CsonPath", (PyObject *)&PyCsonPathType);

  return m;
}
