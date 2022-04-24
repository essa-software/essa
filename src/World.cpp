// keep first!
#include <GL/glew.h>

#include "essagui/EssaGUI.hpp"
#include "Object.hpp"
#include "SimulationView.hpp"
#include "World.hpp"
#include "math/Vector3.hpp"
#include "pyssa/Object.hpp"
#include "util/SimulationClock.hpp"

#include <GL/gl.h>
#include <SFML/Graphics.hpp>
#include <cassert>
#include <iostream>
#include <sstream>

World::World()
    : m_date(Util::SimulationTime::create(1990, 4, 20)) { }

void World::add_object(std::unique_ptr<Object> object) {
    m_object_list.push_back(std::move(object));

    if (m_object_history.set_time(m_date))
        m_object_history.clear_history(m_object_history.get_pos());

    // for_each_object([](Object& obj) {
    //     obj.reset_history();
    // });

    m_object_history.clear_history(m_object_history.get_pos());
}

void World::update(int steps) {
    assert(steps != 0);
    bool reverse = steps < 0;

    for (unsigned i = 0; i < std::abs(steps); i++) {
        if (!m_is_forward_simulated) {
            m_object_history.set_time(m_date);
            if (!reverse) {
                m_date += Util::SimulationClock::duration(m_simulation_seconds_per_tick);

                if (m_object_history.size() > 0 && m_object_history.back()->creation_date() < m_date)
                    m_object_list.push_back(std::move(m_object_history.pop_from_entries()));
            }
            else {
                m_date -= Util::SimulationClock::duration(m_simulation_seconds_per_tick);

                if (m_object_list.size() > 0 && m_object_list.back()->creation_date() > m_date) {
                    m_object_history.push_to_entry(std::move(m_object_list.back()));
                    m_object_list.pop_back();
                }
            }
        }

        for (auto& p : m_object_list)
            p->setup_update_forces();

        for (auto it = m_object_list.begin(); it != m_object_list.end(); it++) {
            auto it2 = it;
            auto& this_object = **it;
            it2++;
            for (; it2 != m_object_list.end(); it2++)
                this_object.update_forces_against(**it2);
        }

        for (auto& p : m_object_list)
            p->update(steps);
    }
}

void World::draw(SimulationView const& view) const {

    {
        WorldDrawScope scope { view, WorldDrawScope::ClearDepth::Yes };
        for (auto& p : m_object_list)
            p->draw(view);
    }
    for (auto& p : m_object_list)
        p->draw_gui(view);
}

Object* World::get_object_by_name(std::string const& name) {
    for (auto& obj : m_object_list) {
        if (obj->name() == name)
            return obj.get();
    }
    return nullptr;
}

void World::reset() {
    m_object_list.clear();
    m_simulation_view->set_focused(nullptr);
    m_date = Util::SimulationTime::create(1990, 4, 20);
    m_object_history.clear_history(0);
    prepare_solar(*this);
}

void World::reset_all_trails() {
    for_each_object([](Object& o) { o.trail().reset(); });
}

void World::clone_for_forward_simulation(World& new_world) const {
    new_world = World();
    new_world.m_is_forward_simulated = true;
    new_world.m_simulation_view = m_simulation_view;
    for (auto& object : m_object_list)
        new_world.add_object(object->clone_for_forward_simulation(new_world));
}

std::ostream& operator<<(std::ostream& out, World const& world) {
    return out << "World (" << world.m_object_list.size() << " objects)";
}

void World::setup_python_bindings(TypeSetup adder) {
    std::cout << "setup_python_bindings!!" << std::endl;
    adder.add_method<&World::python_get_object_by_name>("get_object_by_name");
    adder.add_attribute<&World::python_get_simulation_seconds_per_tick, &World::python_set_simulation_seconds_per_tick>("simulation_seconds_per_tick");
}

PySSA::Object World::python_get_object_by_name(PySSA::Object const& args) {
    const char* name_arg;
    if (!PyArg_ParseTuple(args.python_object(), "s", &name_arg))
        return {};

    auto object = get_object_by_name(name_arg);
    if (object)
        return object->wrap();
    return PySSA::Object::none();
}

PySSA::Object World::python_get_simulation_seconds_per_tick() const {
    return PySSA::Object::create(m_simulation_seconds_per_tick);
}

bool World::python_set_simulation_seconds_per_tick(PySSA::Object const& object) {
    auto maybe_value = object.as_int();
    if (!maybe_value.has_value())
        return false;
    m_simulation_seconds_per_tick = maybe_value.value();
    return true;
}
