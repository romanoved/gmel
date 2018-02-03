#include <Python.h>

#include "util.h"

#include <gnumake.h>

int plugin_is_GPL_compatible;

extern const gmk_floc* reading_file;

static PyObject* GMEL_PY_MAIN_MODULE = NULL;
static PyObject* GMEL_PY_MAIN_DICT = NULL;

void gmel_py_check_initialize(void) {
    if (!GMEL_PY_MAIN_MODULE)
        gmel_error("gmel python subsystem is not initialized");
}

char* gmel_py_initialize(char* func_name, int argc, char** argv) {
    if (GMEL_PY_MAIN_MODULE) return NULL;

    Py_Initialize();

    GMEL_PY_ASSERT(GMEL_PY_MAIN_MODULE = PyImport_AddModule("__main__"));
    GMEL_PY_ASSERT(GMEL_PY_MAIN_DICT = PyModule_GetDict(GMEL_PY_MAIN_MODULE));

    return NULL;
}

char* gmel_py_finalize(char* func_name, int argc, char** argv) {
    if (!GMEL_PY_MAIN_MODULE) return NULL;

    Py_Finalize();

    GMEL_PY_MAIN_MODULE = NULL;
    GMEL_PY_MAIN_DICT = NULL;

    return NULL;
}

char* gmel_py_eval(char* func_name, int argc, char** argv) {
    PyObject* cobj = NULL;
    PyObject* obj = NULL;

    gmel_py_check_initialize();

    char* code_loc = safe_sprintf("<string>");
    if (reading_file)
        code_loc = safe_sprintf("<%s:%lu>", reading_file->filenm,
                                reading_file->lineno);

    cobj = Py_CompileString(argv[0], code_loc, Py_file_input);
    free(code_loc);
    if (!cobj) {
        PyErr_Print();
        gmel_error("py_eval: pycode compilation failed");
    }

    if (!(obj = PyImport_ExecCodeModule("__main__", cobj))) {
        PyErr_Print();
        Py_DECREF(cobj);
        gmel_error("py_eval: pycode execution failed");
    }

    Py_DECREF(cobj);
    Py_DECREF(obj);

    return NULL;
}

char* gmel_py_call(char* func_name, int argc, char** argv) {
    /* argv[0] - python function to call, argv[1:] - python argments */

    PyObject* obj = NULL;
    PyObject* sobj = NULL;
    PyObject* py_args = NULL;
    PyObject* gmel_pycall_wrapper = NULL;
    int i;
    char* buf = NULL;

    gmel_py_check_initialize();

    GMEL_PY_ASSERT(py_args = PyTuple_New(argc - 1));
    for (i = 1; i < argc; ++i) {
#if PY_MAJOR_VERSION == 2
        GMEL_PY_ASSERT(obj = PyString_FromString(argv[i]));
#else
        GMEL_PY_ASSERT(obj = PyUnicode_FromString(argv[i]));
#endif
        GMEL_PY_ASSERT(!PyTuple_SetItem(py_args, i - 1, obj));
    }

    if (!(obj = PyDict_GetItemString(GMEL_PY_MAIN_DICT, argv[0])))
        gmel_error("%s: pyfunc '%s' not found", func_name, argv[0]);

    if (!PyCallable_Check(obj))
        gmel_error("%s: pyfunc '%s' is not callable", func_name, argv[0]);

    if (!(obj = PyObject_CallObject(obj, py_args))) {
        PyErr_Print();
        gmel_error("%s: pyfunc '%s': failed on call", func_name, argv[0]);
    }
    Py_DECREF(py_args);

    GMEL_PY_ASSERT(gmel_pycall_wrapper = PyDict_GetItemString(
                       GMEL_PY_MAIN_DICT, "gmel_pycall_wrapper"));

    sobj = PyObject_CallFunctionObjArgs(gmel_pycall_wrapper, obj, NULL);
    Py_DECREF(obj);
    if (!sobj) {
        Py_DECREF(obj);
        PyErr_Print();
        gmel_error("%s: pyfunc %s: gmel_pycall_wrapper failed", func_name,
                   argv[0]);
    }

    if (sobj == Py_None) {
        Py_DECREF(sobj);
        return NULL;
    }

    if (!PyBytes_Check(sobj)) {
        Py_DECREF(sobj);
        gmel_error("%s: pyfunc %s: expected None or bytes as result", func_name,
                   argv[0]);
    }

    buf = GMEL_ALLOC(PyBytes_GET_SIZE(sobj) + 1);
    memcpy(buf, PyBytes_AS_STRING(sobj), PyBytes_GET_SIZE(sobj));
    buf[PyBytes_GET_SIZE(sobj)] = '\0';

    Py_DECREF(sobj);

    return buf;
}

#if PY_MAJOR_VERSION == 2
#define INIT gmel_py2_gmk_setup
#else
#define INIT gmel_py3_gmk_setup
#endif

int INIT(void) {
    gmk_add_function("py_initialize", (gmk_func_ptr)gmel_py_initialize, 0, 0,
                     0);
    gmk_add_function("py_finalize", (gmk_func_ptr)gmel_py_finalize, 0, 0, 0);
    gmk_add_function("py_eval", (gmk_func_ptr)gmel_py_eval, 1, 1, 0);
    gmk_add_function("py_call", (gmk_func_ptr)gmel_py_call, 1, 0, 0);
    return 1;
}
