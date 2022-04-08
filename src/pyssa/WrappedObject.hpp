#pragma once

#include "Object.hpp"
#include "descrobject.h"
#include "object.h"
#include "objimpl.h"

#include <vector>

namespace PySSA {

// A TypeObject wrapper for specific T. Makes all Python objects
// of this type just handles, so that things they wrap aren't deleted
// by Python interpreter. Should be used for objects that we want to
// be accessible from Python scripts but cannot be created by them.
template<class T>
class WrappedObject {
public:
    struct PythonType {
        PyObject_HEAD
            T* ptr;
    };

    static PyTypeObject& create_type_object();
    Object wrap() {
        auto type = &create_type_object();
        auto object = Object::take(PyObject_New(PyObject, type));
        ((PythonType*)(object.python_object()))->ptr = static_cast<T*>(this);
        return object;
    }

private:
    struct Funcs {
        Funcs() = default;
        Funcs(Funcs const&) = delete;
        Funcs& operator=(Funcs const&) = delete;
        Funcs(Funcs&&) = default;
        Funcs& operator=(Funcs&&) = default;
        std::vector<PyMethodDef> methods;
        std::vector<PyGetSetDef> getters_setters;
    };

protected:
    class FuncsAdder {
    public:
        FuncsAdder(Funcs& funcs)
            : m_funcs(funcs) { }

        using Method = Object (T::*)(Object const& args);
        using Getter = Object (T::*)() const;
        using Setter = bool (T::*)(Object const&);

        template<Method method>
        struct MethodWrapper {
            static PyObject* wrapper(PythonType* self, PyObject* args) {
                return (self->ptr->*method)(Object::take(args)).leak_object();
            }
        };

        template<Getter getter>
        struct GetterWrapper {
            static PyObject* wrapper(PythonType* self, void*) {
                return (self->ptr->*getter)().leak_object();
            }
        };

        template<Setter setter>
        struct SetterWrapper {
            static int wrapper(PythonType* self, PyObject* value, void*) {
                return (self->ptr->*setter)(Object::take(value)) ? 0 : 1;
            }
        };

        template<Method method>
        void add_method(char const* name) const {
            m_funcs.methods.push_back(PyMethodDef { .ml_name = name, .ml_meth = (PyCFunction)MethodWrapper<method>::wrapper, .ml_flags = METH_VARARGS });
        }

        template<Getter getter, Setter setter>
        void add_attribute(const char* attr) const {
            m_funcs.getters_setters.push_back(PyGetSetDef { .name = attr, .get = (::getter)GetterWrapper<getter>::wrapper, .set = (::setter)SetterWrapper<setter>::wrapper, .closure = nullptr });
        }

    private:
        Funcs& m_funcs;
    };

private:
    static Funcs& python_funcs();
};

template<class T>
auto WrappedObject<T>::python_funcs() -> Funcs& {
    static Funcs funcs = []() -> Funcs {
        Funcs funcs2;
        constexpr bool has_setup_python_bindings = requires(FuncsAdder a) { T::setup_python_bindings(a); };
        static_assert(has_setup_python_bindings, "You need to define static void setup_python_bindings(FuncsAdder)");
        if constexpr (has_setup_python_bindings) {
            T::setup_python_bindings(FuncsAdder { funcs2 });
        }
        funcs2.methods.push_back(PyMethodDef { nullptr, nullptr, 0, nullptr });
        funcs2.getters_setters.push_back(PyGetSetDef { nullptr, nullptr, nullptr, nullptr, nullptr });
        return funcs2;
    }();
    return funcs;
}

template<class T>
PyTypeObject& WrappedObject<T>::create_type_object() {
    static PyTypeObject type_object = []() {
        auto& funcs = python_funcs();
        auto type = PyTypeObject { PyVarObject_HEAD_INIT(nullptr, 0) };
        type.tp_name = "WrappedObject";
        type.tp_doc = "TODO";
        type.tp_basicsize = sizeof(T);
        type.tp_itemsize = 0;
        type.tp_flags = Py_TPFLAGS_DEFAULT;
        type.tp_new = PyType_GenericNew;
        type.tp_methods = funcs.methods.data();
        type.tp_getset = funcs.getters_setters.data();
        return type;
    }();
    assert(PyType_Ready(&type_object) >= 0);
    return type_object;
}

}