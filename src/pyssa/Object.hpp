#pragma once

#ifdef ENABLE_PYSSA
#    include <EssaUtil/Color.hpp>
#    include <EssaUtil/Vector.hpp>

#    include <Python.h>
#    include <cstdio>
#    include <iostream>
#    include <optional>
#    include <string>
#    include <utility>
#    include <vector>

namespace PySSA {

class Object {
public:
    Object()
        : m_object(nullptr) { }

    ~Object() {
        Py_XDECREF(m_object);
    }

    Object(Object const& other)
        : m_object(other.m_object) {
        Py_XINCREF(m_object);
    }

    Object& operator=(Object const& other) {
        if (this == &other)
            return *this;
        Py_XDECREF(m_object);
        m_object = other.m_object;
        Py_XINCREF(m_object);
        return *this;
    }

    Object(Object&& other)
        : m_object(std::exchange(other.m_object, nullptr)) { }

    Object& operator=(Object&& other) {
        if (this == &other)
            return *this;
        Py_XDECREF(m_object);
        m_object = std::exchange(other.m_object, nullptr);
        return *this;
    }

    // Create a new Object without sharing a reference (take ownership).
    // Use for all Python object that you created and nothing else!
    static Object take(PyObject* object) {
        Object o;
        o.m_object = object;
        return o;
    }

    // Create a new Object and share a reference.
    static Object share(PyObject* object) {
        Object o;
        o.m_object = object;
        Py_XINCREF(o.m_object);
        return o;
    }

    static Object empty_tuple(Py_ssize_t size) {
        Object o;
        o.m_object = PyTuple_New(size);
        return o;
    }

    static Object none();
    static Object create(Util::UString const&);
    static Object create(int);
    static Object create(double);

    template<class... Args>
    static Object tuple(Args... args) {
        auto tuple = empty_tuple(sizeof...(Args));
        size_t index = 0;
        (tuple.set_tuple_item(index++, args), ...);
        return tuple;
    }

    static Object create(Util::DeprecatedVector3d const&);
    static Object create(Util::Color const&);

    void set_tuple_item(Py_ssize_t i, Object const& object) {
        PyTuple_SetItem(m_object, i, object.share_object());
    }

    // Get access to a Python object without transferring ownership.
    PyObject* python_object() const { return m_object; }

    bool operator!() const { return !m_object; }

    void print() const {
        PyObject_Print(m_object, stdout, 0);
        puts("");
    }

    // Stop owning the object. After that, it's the user responsibility
    // to manage object lifetime.
    PyObject* leak_object() {
        auto object = m_object;
        m_object = nullptr;
        return object;
    }

    // Return an internal Python object, also sharing a reference. Use
    // when calling Python functions that want to share these values
    PyObject* share_object() const {
        Py_XINCREF(m_object);
        return m_object;
    }

    Object get_attribute(Object const& name);
    Object get_attribute(Util::UString const& name) { return get_attribute(Object::create(name)); }
    void set_attribute(Object const& name, Object const& value);
    void set_attribute(Util::UString const& name, Object const& value) { set_attribute(Object::create(name), value); }

    Object call(Object const& args, Object const& kwargs = {}) const;

    std::optional<Util::UString> as_string() const;
    std::optional<int> as_int() const;
    std::optional<double> as_double() const;
    std::optional<std::vector<Object>> as_list() const;
    std::optional<std::vector<Object>> as_tuple() const;
    std::optional<Util::DeprecatedVector3d> as_vector() const;
    std::optional<Util::Color> as_color() const;

    Util::UString str() const;
    Util::UString repr() const;

private:
    PyObject* m_object; // Initialized in Object()
};

}

#endif
