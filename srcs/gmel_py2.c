#include <Python.h>

#include "util.h"

#include <gnumake.h>

int plugin_is_GPL_compatible;

static PyObject* PY2_MAIN = NULL;

void check_init(void) {
    if (!PY2_MAIN) {
        gmk_expand("$(error gmel_py2 is not initialized)");
        abort();
    }
}
char* py2_call(char* func_name, int argc, char** argv) {
    check_init();
    /* argv[0] - python function to call, argv[1:] - python argments */
    PyObject* obj;
    PyObject* sobj;
    PyObject* py_args = NULL;
    int i;

    if (!(py_args = PyTuple_New(argc - 1))) {
        PyErr_Print();
        gmk_expand(save_sprintf("$(error %s: PyTuple_New failed)", func_name));
        abort();
    }
    for (i = 1; i < argc; ++i) {
        if (!(obj = PyString_FromString(argv[i]))) {
            PyErr_Print();
            gmk_expand(save_sprintf("$(error %s: PyString_FromString failed)",
                                    func_name));
            abort();
        }
        if (PyTuple_SetItem(py_args, i - 1, obj)) {
            PyErr_Print();
            gmk_expand(
                save_sprintf("$(error %s: PyTuple_SetItem failed)", func_name));
            abort();
        }
    }

    if (!(obj = PyDict_GetItemString(PyModule_GetDict(PY2_MAIN), argv[0]))) {
        gmk_expand(save_sprintf("$(error %s: no python object '%s')", func_name,
                                argv[0]));
        abort();
    }

    if (!PyCallable_Check(obj)) {
        gmk_expand(save_sprintf("$(error %s: not callable object '%s')",
                                func_name, argv[0]));
        abort();
    }
    if (!(obj = PyObject_CallObject(obj, py_args))) {
        PyErr_Print();
        gmk_expand(save_sprintf("$(error %s: fail on call object '%s')",
                                func_name, argv[0]));
        abort();
    }
    if (!(sobj = PyObject_Str(obj))) {
        Py_DECREF(obj);
        gmk_expand(save_sprintf(
            "$(error %s: fail to convert result of '%s' to string)", func_name,
            argv[0]));
        abort();
    }
    Py_DECREF(obj);

    if (!(PyString_Check(sobj))) {
        Py_DECREF(sobj);
        gmk_expand(
            save_sprintf("$(error %s: expected string as return value of '%s')",
                         func_name, argv[0]));
        abort();
    }

    char* buf = gmel_smalloc("py2_call", PyString_GET_SIZE(sobj) + 1);
    memcpy(buf, PyString_AS_STRING(sobj), PyString_GET_SIZE(sobj));
    buf[PyString_GET_SIZE(sobj)] = '\0';
    Py_DECREF(sobj);

    return buf;
}

char* py2_eval(char* func_name, int argc, char** argv) {
    check_init();
    PyObject* cobj = Py_CompileString(argv[0], "<string>", Py_file_input);
    if (!cobj) {
        PyErr_Print();
        gmk_expand(save_sprintf("$(error py2_eval::Py_CompileString failed)"));
        abort();
    }
    PyObject* obj = PyImport_ExecCodeModule("__main__", cobj);
    Py_DECREF(cobj);
    if (!obj) {
        PyErr_Print();
        gmk_expand(
            save_sprintf("$(error py2_eval::PyImport_ExecCodeModule failed)"));
        abort();
    }
    Py_DECREF(obj);
    return NULL;
}

char* py2_init(char* func_name, int argc, char** argv) {
    if (PY2_MAIN) return NULL;

    Py_SetProgramName("py2make");
    Py_Initialize();

    PY2_MAIN = PyImport_AddModule("__main__");
    if (!PY2_MAIN) {
        PyErr_Print();
        gmk_expand("$(error PyImport_AddModule failed)");
        abort();
    }
    return NULL;
}

char* py2_finalize(char* func_name, int argc, char** argv) {
    check_init();
    Py_Finalize();
    PY2_MAIN = NULL;
    return NULL;
}

int gmel_py2_gmk_setup(void) {
    py2_init(NULL, 0, NULL);
    gmk_add_function("py2_init", (gmk_func_ptr)py2_init, 0, 0, 0);
    gmk_add_function("py2_finalize", (gmk_func_ptr)py2_finalize, 0, 0, 0);
    gmk_add_function("py2_eval", (gmk_func_ptr)py2_eval, 1, 1, 0);
    gmk_add_function("py2_call", (gmk_func_ptr)py2_call, 1, 0, 0);
    return 1;
}
