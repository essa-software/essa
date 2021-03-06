#include "SimulationView.hpp"

#include "World.hpp"
#include "math/Ray.hpp"

#include <EssaGUI/gfx/Window.hpp>
#include <EssaGUI/gui/Application.hpp>
#include <EssaGUI/gui/NotifyUser.hpp>
#include <EssaGUI/gui/TextAlign.hpp>
#include <EssaGUI/gui/WidgetTreeRoot.hpp>
#include <EssaUtil/DelayedInit.hpp>
#include <EssaUtil/UnitDisplay.hpp>
#include <EssaUtil/Vector.hpp>
#include <LLGL/OpenGL/Shader.hpp>
#include <LLGL/OpenGL/Shaders/Basic330Core.hpp>
#include <LLGL/OpenGL/Vertex.hpp>
#include <LLGL/Renderer/Transform.hpp>
#include <LLGL/Window/Mouse.hpp>
#include <cmath>
#include <optional>
#include <sstream>

void SimulationView::handle_event(GUI::Event& event) {
    if (event.type() == llgl::Event::Type::MouseButtonPress) {
        m_prev_mouse_pos = Util::Vector2f { Util::Vector2i { event.event().mouse_button.position } };
        // std::cout << "SV MouseButtonPressed " << Vector3 { m_prev_mouse_pos } << "b=" << (int)event.event().mouseButton.button << std::endl;
        m_prev_drag_pos = m_prev_mouse_pos;
        if (event.event().mouse_button.button == llgl::MouseButton::Left) {
            m_drag_mode = DragMode::Pan;

            m_world.for_each_object([&](Object& obj) {
                auto obj_pos_screen = world_to_screen(obj.render_position());
                auto distance = Util::get_distance(Util::Vector2f { obj_pos_screen.x(), obj_pos_screen.y() }, m_prev_mouse_pos);
                if (distance < 30) {
                    set_focused_object(&obj, GUI::NotifyUser::Yes);
                    return;
                }
            });

            if (m_measure == Measure::Coords)
                m_measure = Measure::None;
            if (m_measure == Measure::Focus && m_focused_object != nullptr) {
                m_measure = Measure::None;
                m_measured = true;
                if (m_on_focus_measure) {
                    m_on_focus_measure(m_focused_object);
                }
                m_focused_object = nullptr;
            }

            event.set_handled();
        }
        else if (llgl::is_mouse_button_pressed(llgl::MouseButton::Right)) {
            m_measure = Measure::None;
            m_drag_mode = DragMode::Rotate;
        }
    }
    else if (event.type() == llgl::Event::Type::MouseScroll) {
        // TODO: Check mouse wheel
        if (event.event().mouse_scroll.delta > 0)
            apply_zoom(1 / 1.1);
        else
            apply_zoom(1.1);
    }
    else if (event.type() == llgl::Event::Type::MouseMove) {
        Util::Vector2f mouse_pos { Util::Vector2i { event.event().mouse_move.position } };
        m_prev_mouse_pos = mouse_pos;
        // std::cout << "SV MouseMoved " << Vector3 { m_prev_mouse_pos } << std::endl;

        // DRAG
        auto drag_delta = m_prev_drag_pos - mouse_pos;

        if (m_drag_mode != DragMode::None && !m_is_dragging && (std::abs(drag_delta.x()) > 20 || std::abs(drag_delta.y()) > 20)) {
            m_is_dragging = true;

            if (m_drag_mode == DragMode::Pan && m_focused_object) {
                m_focused_object = nullptr;
                m_offset.z() = 0;
            }
        }

        if (m_is_dragging) {
            switch (m_drag_mode) {
            case DragMode::Pan: {
                Util::Vector3d qad_delta { -drag_delta.x(), drag_delta.y(), 0 };
                set_offset(offset() + Util::Vector3d { llgl::Transform {}.rotate_z(-real_yaw()).transform_point(Util::Vector3f { qad_delta }) * scale() / 320 });
                break;
            }
            case DragMode::Rotate: {
                auto sizes = window().size();
                m_yaw -= drag_delta.x() / sizes.x() * M_PI;
                m_pitch += drag_delta.y() / sizes.y() * M_PI;
                break;
            }
            default:
                break;
            }
            m_prev_drag_pos = mouse_pos;
        }

        // Coord measure
        if (m_measure == Measure::Coords) {
            auto object_pos_in_world_space = screen_to_world_on_grid(mouse_pos);
            if (object_pos_in_world_space) {
                m_measured = true;
                if (m_on_coord_measure) {
                    m_on_coord_measure(*object_pos_in_world_space * AU);
                }
            }
        }
    }
    else if (event.type() == llgl::Event::Type::MouseButtonRelease) {
        // std::cout << "SV MouseButtonReleased " << Vector3 { m_prev_mouse_pos } << "b=" << (int)event.event().mouseButton.button << std::endl;
        m_is_dragging = false;
        m_drag_mode = DragMode::None;
    }
    else if (event.type() == llgl::Event::Type::KeyPress && m_allow_change_speed) {
        if (event.event().key.shift) {
            if (m_speed != 0)
                return;
            if (event.event().key.keycode == llgl::KeyCode::Right)
                m_world.update(1);
            else if (event.event().key.keycode == llgl::KeyCode::Left)
                m_world.update(-1);
        }
        else {
            if (event.event().key.keycode == llgl::KeyCode::Right) {
                if (m_speed > 0)
                    m_speed *= 2;
                else if (m_speed == 0)
                    m_speed = 1;
                else
                    m_speed /= 2;
            }
            else if (event.event().key.keycode == llgl::KeyCode::Left) {
                if (m_speed < 0)
                    m_speed *= 2;
                else if (m_speed == 0)
                    m_speed = -1;
                else
                    m_speed /= 2;
            }
        }
    }
}

std::optional<Util::Vector3d> SimulationView::screen_to_world_on_grid(Util::Vector2f screen) const {
    auto clip_space = screen_to_clip_space(screen);
    Math::Ray ray { { clip_space.x(), clip_space.y(), 0 }, { clip_space.x(), clip_space.y(), 1 } };

    // Transform a grid plane (z = 0) to the clip space.
    Math::Plane plane = Math::Plane(0, 0, 1, 0).transformed(matrix());
    /// std::cout << "[Coord] Mouse(Clip): " << clip_space << std::endl;

    // Calculate intersection of mouse ray and the grid. This will be our object position in clip space.
    auto object_pos_in_clip_space = ray.intersection_with_plane(plane);
    if (object_pos_in_clip_space) {
        // Go back to world coordinates to get actual object position.
        auto object_pos_in_world_space = llgl::Transform { matrix().inverted().convert<float>() }.transform_point(Util::Vector3f { object_pos_in_clip_space.value() });

        // std::cout << "[Coord] Object(Clip)->Object(World): " << *object_pos_in_clip_space << " -> " << object_pos_in_world_space << std::endl;

        return Util::Vector3d { object_pos_in_world_space };
    }
    return {};
}

void SimulationView::set_focused_object(Object* obj, GUI::NotifyUser notify_user) {
    m_focused_object = obj;
    if (notify_user == GUI::NotifyUser::Yes && m_focused_object && on_change_focus)
        on_change_focus(m_focused_object);
}

void SimulationView::draw_grid(GUI::Window& window) const {
    constexpr float zoom_step_exponent = 2;
    auto spacing = std::pow(zoom_step_exponent, std::round(std::log2(m_zoom / 2) / std::log2(zoom_step_exponent)));
    constexpr int major_gridline_interval = 4;
    auto major_gridline_spacing = spacing * major_gridline_interval;
    {
        WorldDrawScope scope(*this);
        float bounds = 50 * spacing;
        // Vector3 start_coords = screen_to_world({ 0, 0 });
        // start_coords.x -= std::remainder(start_coords.x, spacing * major_gridline_interval) + spacing;
        // start_coords.y -= std::remainder(start_coords.y, spacing * major_gridline_interval) + spacing;
        // Vector3 end_coords = screen_to_world({ size().x, size().y });
        Util::Vector3d start_coords = { -bounds, -bounds, 0 };
        start_coords.x() -= std::round(m_offset.x() / major_gridline_spacing) * major_gridline_spacing;
        start_coords.y() -= std::round(m_offset.y() / major_gridline_spacing) * major_gridline_spacing;
        Util::Vector3d end_coords = { bounds, bounds, 0 };
        end_coords.x() -= std::round(m_offset.x() / major_gridline_spacing) * major_gridline_spacing;
        end_coords.y() -= std::round(m_offset.y() / major_gridline_spacing) * major_gridline_spacing;
        Util::Vector3d center_coords = (start_coords + end_coords) / 2.0;

        Util::Color const major_grid_line_color { 87, 87, 108 };
        Util::Color const grid_line_color { 25, 25, 37 };

        // TODO: Create proper API for it.
        std::vector<llgl::Vertex> vertices;

        int index = 0;

        auto blend_color = [](Util::Color start, Util::Color end, float fac) {
            return Util::Color {
                static_cast<uint8_t>(start.r * (1 - fac) + end.r * fac),
                static_cast<uint8_t>(start.g * (1 - fac) + end.g * fac),
                static_cast<uint8_t>(start.b * (1 - fac) + end.b * fac),
                static_cast<uint8_t>(start.a * (1 - fac) + end.a * fac),
            };
        };

        // FIXME: Calculate bounds depending on window size instead of hardcoding them
        // TODO: Add real fog shader instead of THIS thing
        for (double x = start_coords.x(); x < end_coords.x(); x += spacing) {
            auto color = index % major_gridline_interval == 2 ? major_grid_line_color : grid_line_color;
            vertices.push_back(llgl::Vertex { .position = { static_cast<float>(x), static_cast<float>(start_coords.y()), 0 }, .color = Util::Colors::transparent });
            double factor = std::abs(0.5 - (x - start_coords.x()) / (end_coords.x() - start_coords.x())) * 2;
            auto center_color = blend_color(color, Util::Colors::transparent, factor);
            vertices.push_back(llgl::Vertex { .position = { static_cast<float>(x), static_cast<float>(center_coords.y()), 0 }, .color = center_color });
            // FIXME: Make this duplicate vertex not needed
            vertices.push_back(llgl::Vertex { .position = { static_cast<float>(x), static_cast<float>(center_coords.y()), 0 }, .color = center_color });
            vertices.push_back(llgl::Vertex { .position = { static_cast<float>(x), static_cast<float>(end_coords.y()), 0 }, .color = Util::Colors::transparent });
            index++;
        }
        index = 0;
        for (double y = start_coords.y(); y < end_coords.y(); y += spacing) {
            auto color = index % major_gridline_interval == 2 ? major_grid_line_color : grid_line_color;
            vertices.push_back(llgl::Vertex { .position = { static_cast<float>(start_coords.x()), static_cast<float>(y), 0 }, .color = Util::Colors::transparent });
            double factor = std::abs(0.5 - (y - start_coords.y()) / (end_coords.y() - start_coords.y())) * 2;
            auto center_color = blend_color(color, Util::Colors::transparent, factor);
            vertices.push_back(llgl::Vertex { .position = { static_cast<float>(center_coords.x()), static_cast<float>(y), 0 }, .color = center_color });
            // FIXME: Make this duplicate vertex not needed
            vertices.push_back(llgl::Vertex { .position = { static_cast<float>(center_coords.x()), static_cast<float>(y), 0 }, .color = center_color });
            vertices.push_back(llgl::Vertex { .position = { static_cast<float>(end_coords.x()), static_cast<float>(y), 0 }, .color = Util::Colors::transparent });
            index++;
        }

        window.draw_vertices(llgl::opengl::PrimitiveType::Lines, vertices);
    }

    // guide
    Util::Vector2f guide_start { size().x() - 200.f, size().y() - 30.f };
    // HACK: this *100 should be calculated from perspective somehow
    Util::Vector2f guide_end = guide_start - Util::Vector2f(spacing * 300 / scale(), 0);
    std::array<llgl::Vertex, 6> guide;
    Util::Color const guide_color { 127, 127, 127 };
    guide[0] = llgl::Vertex { .position = Util::Vector3f(guide_start, 0), .color = guide_color };
    guide[1] = llgl::Vertex { .position = Util::Vector3f(guide_end, 0), .color = guide_color };
    guide[2] = llgl::Vertex { .position = Util::Vector3f(guide_start - Util::Vector2f(0, 5), 0), .color = guide_color };
    guide[3] = llgl::Vertex { .position = Util::Vector3f(guide_start + Util::Vector2f(0, 5), 0), .color = guide_color };
    guide[4] = llgl::Vertex { .position = Util::Vector3f(guide_end - Util::Vector2f(0, 5), 0), .color = guide_color };
    guide[5] = llgl::Vertex { .position = Util::Vector3f(guide_end + Util::Vector2f(0, 5), 0), .color = guide_color };
    window.draw_vertices(llgl::opengl::PrimitiveType::Lines, guide);

    // FIXME: UB on size_t conversion
    GUI::TextDrawOptions guide_text;
    guide_text.font_size = 15;
    guide_text.text_align = GUI::Align::Center;
    window.draw_text_aligned_in_rect(Util::unit_display(spacing / 2 / zoom_step_exponent * AU, Util::Quantity::Length).to_string(),
        { guide_start - Util::Vector2f(0, 10), guide_end - guide_start }, GUI::Application::the().font(), guide_text);
}

llgl::Transform SimulationView::camera_transform() const {
    return llgl::Transform {}
        .translate({ 0, 0, -scale() })
        .rotate_x(m_pitch)
        .rotate_z(m_yaw)
        .translate(Util::Vector3f { m_offset });
}

llgl::View SimulationView::view() const {
    llgl::View view;
    view.set_perspective({ m_fov.rad(), size().x() / size().y(), 0.1 * scale(), 1000 * scale() });
    view.set_viewport(Util::Recti { rect() });
    return view;
}

Util::Matrix4x4d SimulationView::matrix() const {
    return (view().matrix() * camera_transform().matrix()).convert<double>();
}

Util::Vector3f SimulationView::world_to_screen(Util::Vector3d local_space) const {
    // https://learnopengl.com/Getting-started/Coordinate-Systems
    auto clip_space = matrix() * Util::Vector4d { local_space };
    clip_space /= clip_space.w();
    return Util::Vector3f { clip_space_to_screen(Util::Vector3d { clip_space }), clip_space.z() };
}

void SimulationView::draw(GUI::Window& window) const {
    if (m_show_grid)
        draw_grid(window);
    m_world.draw(*this);

    switch (m_measure) {
    case Measure::Focus: {
        auto sizes = size();
        std::array<llgl::Vertex, 4> lines;
        lines[0] = llgl::Vertex { .position = Util::Vector3f { 0, static_cast<float>(m_prev_mouse_pos.y()), 0 }, .color = Util::Colors::green };
        lines[1] = llgl::Vertex { .position = Util::Vector3f { sizes.x(), static_cast<float>(m_prev_mouse_pos.y()), 0 }, .color = Util::Colors::green };
        lines[2] = llgl::Vertex { .position = Util::Vector3f { static_cast<float>(m_prev_mouse_pos.x()), 0, 0 }, .color = Util::Colors::green };
        lines[3] = llgl::Vertex { .position = Util::Vector3f { static_cast<float>(m_prev_mouse_pos.x()), sizes.y(), 0 }, .color = Util::Colors::green };
        window.draw_vertices(llgl::opengl::PrimitiveType::Lines, lines);
        break;
    }
    default:
        break;
    }

    std::ostringstream oss;
    oss << m_world.date();
    if (m_speed == 0)
        oss << " (Paused";
    else
        oss << " (" << std::abs(m_speed) << "x";

    if (m_speed < 0)
        oss << ", Reversed";
    oss << ")";

    std::ostringstream debugoss;
    auto mp = llgl::mouse_position();
    debugoss << "s=" << scale() << std::endl;
    debugoss << "off=" << offset() << std::endl;
    debugoss << "yaw=" << m_yaw << " $ " << m_yaw_from_object << std::endl;
    debugoss << "pitch=" << m_pitch << " $ " << m_pitch_from_object << std::endl;

    GUI::TextDrawOptions debug_text;
    debug_text.fill_color = Util::Colors::white;
    debug_text.font_size = 15;
    window.draw_text(Util::UString { debugoss.str() }, GUI::Application::the().fixed_width_font(), { 600, 20 }, debug_text);
}

void SimulationView::pause_simulation(bool state) {
    if (!state) {
        m_speed = m_saved_speed;
    }
    else {
        m_saved_speed = m_speed;
        m_speed = 0;
    }
}

void SimulationView::update() {
    // FIXME: This doesn't quite match here like speed (The same
    //        comment about Simulation object)
    if (m_speed != 0)
        m_world.update(m_speed * m_iterations);

    // Handle focus
    if (m_focused_object) {
        set_offset(-m_focused_object->render_position());

        if (m_focused_object->most_attracting_object() && m_fixed_rotation_on_focus) {
            Util::Vector3d a = m_focused_object->pos() - m_focused_object->most_attracting_object()->pos();

            m_pitch_from_object = std::atan2(a.y(), a.z()) - M_PI / 2;
            m_yaw_from_object = std::atan2(a.y(), a.x()) + M_PI / 2;

            if (a.y() < 0)
                m_pitch_from_object -= M_PI;
        }
    }

    if (m_focused_object != nullptr && if_focused)
        if_focused();
}

Object* SimulationView::focused_object() const {
    if (m_focused_object != nullptr)
        return m_focused_object;
    return {};
}

void SimulationView::apply_states() const {
    window().set_view_matrix(camera_transform().matrix());
    window().set_view(view());
}

Util::Vector2f SimulationView::clip_space_to_screen(Util::Vector3d clip_space) const {
    return { (clip_space.x() + 1) / 2 * size().x(), size().y() - (clip_space.y() + 1) / 2 * size().y() };
}

Util::Vector3d SimulationView::screen_to_clip_space(Util::Vector2f viewport) const {
    return { (viewport.x() - size().x() / 2.0) * 2 / size().x(), -(viewport.y() - size().y() / 2.0) * 2 / size().y(), 1 };
}

#ifdef ENABLE_PYSSA

void SimulationView::setup_python_bindings(TypeSetup type_setup) {
    type_setup.add_method<&SimulationView::python_reset>("reset", "Reset view transform");
    type_setup.add_attribute<&SimulationView::python_get_offset, &SimulationView::python_set_offset>("offset");
    type_setup.add_attribute<&SimulationView::python_get_fov, &SimulationView::python_set_fov>("fov", "Field of view");
    type_setup.add_attribute<&SimulationView::python_get_yaw, &SimulationView::python_set_yaw>("yaw");
    type_setup.add_attribute<&SimulationView::python_get_pitch, &SimulationView::python_set_pitch>("pitch");
    type_setup.add_attribute<&SimulationView::python_get_zoom, &SimulationView::python_set_zoom>("zoom");
    type_setup.add_attribute<&SimulationView::python_get_focused_object, &SimulationView::python_set_focused_object>("focused_object");
}

PySSA::Object SimulationView::python_reset(PySSA::Object const&, PySSA::Object const&) {
    reset();
    return PySSA::Object::none();
}

PySSA::Object SimulationView::python_get_offset() const {
    return PySSA::Object::create(m_offset);
}

bool SimulationView::python_set_offset(PySSA::Object const& object) {
    auto v = object.as_vector();
    if (!v)
        return false;
    m_offset = *v;
    return true;
}

PySSA::Object SimulationView::python_get_fov() const {
    return PySSA::Object::create(m_fov.rad());
}

bool SimulationView::python_set_fov(PySSA::Object const& object) {
    auto v = object.as_double();
    if (!v)
        return false;
    m_fov = Util::Angle(*v, Util::Angle::Unit::Deg);
    return true;
}

PySSA::Object SimulationView::python_get_yaw() const {
    return PySSA::Object::create(m_yaw);
}

bool SimulationView::python_set_yaw(PySSA::Object const& object) {
    auto v = object.as_double();
    if (!v)
        return false;
    m_yaw = *v;
    return true;
}

PySSA::Object SimulationView::python_get_pitch() const {
    return PySSA::Object::create(m_pitch);
}

bool SimulationView::python_set_pitch(PySSA::Object const& object) {
    auto v = object.as_double();
    if (!v)
        return false;
    m_pitch = *v;
    return true;
}

PySSA::Object SimulationView::python_get_world() const {
    return m_world.wrap();
}

PySSA::Object SimulationView::python_get_zoom() const {
    return PySSA::Object::create(m_zoom);
}

bool SimulationView::python_set_zoom(PySSA::Object const& object) {
    auto v = object.as_double();
    if (!v)
        return false;
    m_zoom = *v;
    return true;
}

PySSA::Object SimulationView::python_get_focused_object() const {
    return m_focused_object ? m_focused_object->wrap() : PySSA::Object::none();
}

bool SimulationView::python_set_focused_object(PySSA::Object const& object) {
    auto v = Object::get(object);
    if (!v)
        return false;
    set_focused_object(v);
    return true;
}

#endif

WorldDrawScope const* s_current_draw_scope = nullptr;

void WorldDrawScope::verify() {
    assert(s_current_draw_scope);
}

WorldDrawScope const* WorldDrawScope::current() {
    return s_current_draw_scope;
}

WorldDrawScope::WorldDrawScope(SimulationView const& view, ClearDepth clear_depth, llgl::opengl::Shader* custom_shader)
    : m_simulation_view(view)
    , m_previous_view(view.window().view()) {

    if (s_current_draw_scope)
        return;

    static llgl::opengl::shaders::Basic330Core basic_shader;

    view.window().set_shader(custom_shader ? custom_shader : &basic_shader);
    m_simulation_view.apply_states();

    glEnable(GL_DEPTH_TEST);

    if (clear_depth == ClearDepth::Yes)
        glClear(GL_DEPTH_BUFFER_BIT);

    m_parent = s_current_draw_scope;
    s_current_draw_scope = this;
}

WorldDrawScope::~WorldDrawScope() {
    s_current_draw_scope = m_parent;
    if (s_current_draw_scope)
        return;

    glDisable(GL_DEPTH_TEST);
    m_simulation_view.window().set_shader(nullptr);
    m_simulation_view.window().set_view(m_previous_view);
}
