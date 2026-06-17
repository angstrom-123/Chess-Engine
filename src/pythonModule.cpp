#include <Python.h>

#include <cstdint>
#include "libchess/api.h"

// Wrappers

namespace py {
    static PyObject *Add(PyObject *self, PyObject *args)
    {
        int32_t a, b;
        if (!PyArg_ParseTuple(args, "ii", &a, &b)) {
            return nullptr;
        }
        return PyLong_FromLong(libchess::Add(a, b));
    }
}

// Method Table

static PyMethodDef libchessMethods[] = {
    { "add", py::Add, METH_VARARGS, "Add two numbers" },
    { nullptr, nullptr, 0, nullptr }
};

// Module Definition

static PyModuleDef libchessModule = {
    PyModuleDef_HEAD_INIT,
    "libchess",
    nullptr,
    -1,
    libchessMethods
};

// Module Init

PyMODINIT_FUNC PyInit_libchess(void)
{
    return PyModule_Create(&libchessModule);
}
