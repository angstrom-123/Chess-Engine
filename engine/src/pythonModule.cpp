#include <Python.h>
#include <cstring>

#include "libchess/board.h"
#include "methodobject.h"
#include "move.h"
#include "pytypedefs.h"
#include "unicodeobject.h"

// Wrappers

namespace py {
    struct Board {
        PyObject_HEAD
        libchess::Board* board;
    };

    static void BoardDealloc(Board *self)
    {
        delete self->board;
        Py_TYPE(self)->tp_free(reinterpret_cast<PyObject *>(self));
    }

    static PyObject *BoardNew(PyTypeObject *type, PyObject *args, PyObject *kwards)
    {
        Board *self = reinterpret_cast<Board *>(type->tp_alloc(type, 0));
        if (self)
            self->board = nullptr;
        return reinterpret_cast<PyObject *>(self);
    }

    static int BoardInit(Board *self, PyObject *args, PyObject *kwargs)
    {
        char *fen = nullptr;
        if (!PyArg_ParseTuple(args, "|s", &fen))
            return -1;
        self->board = new libchess::Board(fen);
        return 0;
    }

    // Methods

    // NOTE: Use Py_RETURN_NONE macro to return nothing

    static PyObject *BoardRepr(Board *self)
    {
        if (!self->board)
            return nullptr;
        std::string result;
        self->board->Show(result);
        return PyUnicode_FromString(result.c_str());
    }
    
    static PyObject *BoardHasError(Board *self, PyObject *args)
    {
        if (!self->board)
            return nullptr;
        return PyBool_FromLong(self->board->HasError());
    }

    static PyObject *BoardGetError(Board *self, PyObject *args)
    {
        if (!self->board)
            return nullptr;
        const char *error = self->board->GetError();
        if (error)
            return PyUnicode_FromString(self->board->GetError());
        Py_RETURN_NONE;
    }

    static PyObject *BoardGo(Board *self, PyObject *args)
    {
        if (!self->board)
            return nullptr;
        uint64_t ms = 1000;
        if (!PyArg_ParseTuple(args, "K", &ms))
            return nullptr;
        Move move = self->board->Go(ms);
        return PyUnicode_FromString(move.ToLAN().chars);
    }

    static PyObject *BoardMakeMove(Board *self, PyObject *args)
    {
        if (!self->board)
            return nullptr;
        char *lan = nullptr;
        if (!PyArg_ParseTuple(args, "s", &lan))
            return nullptr;

        LongAlgebraicMove move = LongAlgebraicMove::FromChars(lan);
        if (!LongAlgebraicMove::IsValid(move))
            return nullptr;
        self->board->MakeMove(move);
        Py_RETURN_NONE;
    }

    // Board Method Table

    static PyMethodDef boardMethods[] = {
        { "has_error", reinterpret_cast<PyCFunction>(py::BoardHasError), METH_NOARGS, "Check if the board has an error." },
        { "get_error", reinterpret_cast<PyCFunction>(py::BoardGetError), METH_NOARGS, "Gets the current error if there is one." },
        { "go", reinterpret_cast<PyCFunction>(py::BoardGo), METH_VARARGS, "Find the best move on the current board within the given time." },
        { "make_move", reinterpret_cast<PyCFunction>(py::BoardMakeMove), METH_VARARGS, "Apply a move in Long Algebraic Notation to update game state." },
        { nullptr, nullptr, 0, nullptr }
    };

    // Board Type Definition

    static PyTypeObject boardType = {
        .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
        .tp_name = "libchess.Board",
        .tp_basicsize = sizeof(libchess::Board),
        .tp_itemsize = 0,
        .tp_dealloc = reinterpret_cast<destructor>(BoardDealloc),
        .tp_repr = reinterpret_cast<reprfunc>(BoardRepr),
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_doc = "Chess board class",
        .tp_methods = boardMethods,
        .tp_init = reinterpret_cast<initproc>(BoardInit),
        .tp_new = BoardNew,
    };
}

// Module Method Table

static PyMethodDef libchessMethods[] = {
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
    PyObject * module = PyModule_Create(&libchessModule);
    if (!module)
        return nullptr;

    if (PyType_Ready(&py::boardType) < 0)
        return nullptr;

    Py_INCREF(&py::boardType);
    PyModule_AddObject(module, "Board", reinterpret_cast<PyObject *>(&py::boardType));

    return module;
}
