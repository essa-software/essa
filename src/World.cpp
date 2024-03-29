// keep first!
#include <EssaUtil/Error.hpp>
#include <EssaUtil/GenericParser.hpp>
#include <GL/glew.h>

#include "ConfigLoader.hpp"
#include "Object.hpp"
#include "SimulationView.hpp"
#include "World.hpp"
#include "essagui/EssaGUI.hpp"
#include "pyssa/Object.hpp"
#include "pyssa/TupleParser.hpp"

#include <EssaUtil/SimulationClock.hpp>
#include <EssaUtil/Vector.hpp>

#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>

using namespace std::chrono_literals;

World::World()
    : m_date(Util::SimulationTime::create(1990, 4, 20)) {
    m_start_date = m_date;
}

void World::add_object(std::unique_ptr<Object> object) {
    object->m_world = this;
    object->m_creation_date = m_date;
    m_object_list.push_back(std::move(object));

    if (m_object_history.set_time(m_date))
        m_object_history.clear_history(m_object_history.get_pos());

    m_object_list.remove_if([&](std::unique_ptr<Object>& obj) {
        return obj->creation_date() > m_date;
    });
}

void World::set_forces() {
    for (auto& p : m_object_list) {
        if (!p->deleted())
            p->clear_forces();
    }

    for (auto it = m_object_list.begin(); it != m_object_list.end(); it++) {
        auto it2 = it;
        auto& this_object = **it;
        it2++;
        for (; it2 != m_object_list.end(); it2++) {
            if (!this_object.deleted() && !(*it2)->deleted())
                this_object.update_forces_against(**it2);
        }
    }
}

void World::update_history_and_date(bool reverse) {
    if (!m_is_forward_simulated) {
        m_object_history.set_time(m_date);
        std::unique_ptr<Object>& last_created = m_object_list.back();

        if (!reverse) {
            m_date += Util::SimulationClock::duration(m_simulation_seconds_per_tick);

            if (m_object_history.size() > 0) {
                if (last_created->creation_date() <= m_date) {
                    m_object_list.push_back(std::move(m_object_history.pop_from_entries()));
                }
            }
        }
        else {
            m_date -= Util::SimulationClock::duration(m_simulation_seconds_per_tick);

            if (m_object_history.size() > 0) {
                if (last_created->creation_date() > m_date) {
                    m_object_history.push_to_entry(std::move(m_object_list.back()));
                    m_object_list.pop_back();
                }
            }
        }
    }
}

void World::update(int steps) {
    assert(steps != 0);
    bool reverse = steps < 0;

    for (unsigned i = 0; i < std::abs(steps); i++) {
        update_history_and_date(reverse);

        for (auto& obj : m_object_list)
            obj->before_update();

        // The algorithm used is Leapfrog KDK
        // http://courses.physics.ucsd.edu/2019/Winter/physics141/Lectures/Lecture2/volker.pdf
        double step = m_simulation_seconds_per_tick;
        double halfStep = (step / 2.0);

        double mul = (steps >= 0) ? 1 : -1;

        // calculate forces/accelerations based on current postions
        this->set_forces();

        for (auto& obj : m_object_list) // for each celestial body
        {
            if (obj->deleted())
                continue;

            // Leapfrog algorithm, step 1
            obj->set_vel(obj->vel() + obj->acc() * halfStep * mul);

            // Leapfrog algorithm, step 2
            obj->set_pos(obj->pos() + obj->vel() * step * mul);
        }

        // calculate the forces using the new positions
        this->set_forces();

        for (auto& obj : m_object_list) // for each celestial body
        {
            if (obj->deleted())
                continue;

            // Leapfrog algorithm, step 3
            obj->set_vel(obj->vel() + obj->acc() * halfStep * mul);

            obj->update(m_simulation_seconds_per_tick);

            // std::cerr << m_date.time_since_epoch().count() << ";" << obj->name() << ";" << obj->pos() << ";" << obj->vel() << ";" << std::endl;
        }

        // if ((m_date - m_start_date).count() > 24 * 60 * 60 * 50) {
        //     std::cout << "FINISHED" << std::endl;
        //     _exit(0);
        // }
    }
}

bool World::exist_object_with_name(Util::UString const& name) const {
    for (const auto& obj : m_object_list) {
        if (obj->name() == name && !obj->deleted())
            return true;
    }

    return false;
}

void World::draw(Gfx::Painter& painter, SimulationView const& view) const {
    {
        GUI::WorldDrawScope scope { painter, GUI::WorldDrawScope::ClearDepth::Yes };
        for (auto& p : m_object_list) {
            if (!p->deleted())
                p->draw(painter, view);
        }
    }
    for (auto& p : m_object_list)
        if (!p->deleted())
            p->draw_gui(painter, view);
}

Object* World::get_object_by_name(Util::UString const& name) {
    for (auto& obj : m_object_list) {
        if (obj->name() == name)
            return obj.get();
    }
    return nullptr;
}

void World::reset(std::optional<std::string> const& filename) {
    if (on_reset)
        on_reset();

    m_object_list.clear();
    m_simulation_view->set_focused_object(nullptr);
    m_date = Util::SimulationTime::create(1990, 4, 20);
    m_object_history.clear_history(0);
    m_light_source = nullptr;

    auto load = [this, &filename]() -> Config::ErrorOr<void> {
        auto config = TRY(ConfigLoader::load(*filename, *this));
        TRY(config.apply(*this));
        return {};
    };

    if (filename) {
        auto maybe_error = load();
        if (maybe_error.is_error()) {
            std::visit(
                Util::Overloaded {
                    [](Util::ParseError const& error) {
                        fmt::print("World loading failed: {} at {}:{}\n", error.message, error.location.start.line + 1, error.location.start.column + 1);
                    },
                    [](Util::OsError const& error) {
                        fmt::print("World loading failed: {}: {}\n", error.function, strerror(error.error));
                    },
                },
                maybe_error.release_error_variant());
        }
    }
}

void World::reset_all_trails() {
    for_each_object([](Object& o) { o.trail().reset(); });
}

void World::delete_object_by_ptr(Object* ptr) {
    m_object_list.remove_if([ptr](std::unique_ptr<Object>& obj) { return obj.get() == ptr; });
    m_simulation_view->set_focused_object(nullptr);

    for (auto& o : m_object_list) {
        if (o->most_attracting_object() == ptr)
            o->delete_most_attracting_object();
    }

    if (m_light_source == ptr)
        m_light_source = nullptr;
}

std::unique_ptr<Object>& World::find_object_by_ptr(Object* ptr) {
    for (auto& o : m_object_list) {
        if (o.get() == ptr)
            return o;
    }

    return m_object_list.back();
}

void World::clone_for_forward_simulation(World& new_world) const {
    new_world = World();
    new_world.m_is_forward_simulated = true;
    new_world.m_simulation_view = m_simulation_view;
    for (auto& object : m_object_list) {
        if (!object->deleted())
            new_world.add_object(object->clone_for_forward_simulation());
    }
}

std::ostream& operator<<(std::ostream& out, World const& world) {
    return out << "World (" << world.m_object_list.size() << " objects)";
}

#ifdef ENABLE_PYSSA

void World::setup_python_bindings(TypeSetup adder) {
    adder.add_method<&World::python_get_object_by_name>("get_object_by_name", "Returns an object that has the name given in argument.");
    adder.add_method<&World::python_add_object>("add_object", "Adds an object to the World.");
    adder.add_attribute<&World::python_get_simulation_seconds_per_tick, &World::python_set_simulation_seconds_per_tick>("simulation_seconds_per_tick",
        "Sets how much simulation seconds passes per tick");
}

PySSA::Object World::python_add_object(PySSA::Object const& args, PySSA::Object const& kwargs) {
    Object* object = nullptr;
    if (!PySSA::parse_arguments(args, kwargs, "O!", PySSA::Arg::Arg { PySSA::Arg::CheckedType<Object> { object }, "object" }))
        return {};
    object->python_stop_owning();
    add_object(std::unique_ptr<Object>(object));
    return PySSA::Object::none();
}

PySSA::Object World::python_get_object_by_name(PySSA::Object const& args, PySSA::Object const& kwargs) {
    Util::UString name = "test";
    if (!PySSA::parse_arguments(args, kwargs, "s", PySSA::Arg::Arg { &name, "name" }))
        return {};

    auto object = get_object_by_name(name);
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

#endif
