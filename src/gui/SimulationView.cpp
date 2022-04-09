#include "SimulationView.hpp"

#include "../Transform.hpp"
#include "../World.hpp"
#include "GUI.hpp"
#include <GL/gl.h>
#include <GL/glu.h>
#include <SFML/System/Vector2.hpp>
#include <cmath>
#include <sstream>

void SimulationView::start_focus_measure() {
    if (m_focused_object != nullptr) {
        m_focused_object->m_focused = false;
        m_focused_object = nullptr;
    }
    m_focus_measure = true;
}

void SimulationView::handle_event(Event& event) {
    m_changed = false;
    if (event.type() == sf::Event::MouseButtonPressed) {
        m_prev_mouse_pos = { static_cast<float>(event.event().mouseButton.x), static_cast<float>(event.event().mouseButton.y) };
        if (event.event().mouseButton.button == sf::Mouse::Left) {
            m_dragging = true;
            // FIXME: This requires more advanced math to work properly.
            // m_world.for_each_object([&](Object& object) {
            //     if(object.hover(*this, world_click_pos))
            //     {
            //         if(m_focused_object != nullptr)
            //             m_focused_object->m_focused = false;
            //         m_focused_object = &object;
            //         m_focused_object->m_focused = true;
            //     }
            // });
            // if (m_coord_measure) {
            //     m_coord_measure = false;
            //     m_measured = true;
            //     if (on_coord_measure) {
            //         on_coord_measure(m_prev_pos);
            //     }
            // }
            // else if (m_focus_measure && m_focused_object != nullptr) {
            //     m_focus_measure = false;
            //     m_measured = true;
            //     if (on_focus_measure) {
            //         on_focus_measure(m_focused_object);
            //     }
            //     m_focused_object->m_focused = false;
            //     m_focused_object = nullptr;
            // }
            event.set_handled();
        }
        else if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
            if (m_focused_object) {
                m_focused_object->m_focused = false;
                m_focused_object = nullptr;
            }

            m_coord_measure = false;
            m_focus_measure = false;

            m_rotating = true;
        }
    }
    else if (event.type() == sf::Event::MouseWheelScrolled) {
        if (event.event().mouseWheelScroll.delta > 0)
            apply_zoom(1.1);
        else
            apply_zoom(1 / 1.1);
        m_changed = true;
    }
    else if (event.type() == sf::Event::MouseMoved) {
        sf::Vector2f mouse_pos { static_cast<float>(event.event().mouseMove.x), static_cast<float>(event.event().mouseMove.y) };
        if (m_dragging) {
            auto delta_pos = m_prev_mouse_pos - mouse_pos;
            // qad from "quick and dirty".
            // FIXME: Use plane projection or sth better???
            Vector3 qad_delta { delta_pos.x, -delta_pos.y, 0 };
            set_offset(offset() + Transform::rotation_around_y(m_rotate_y) * Transform::rotation_around_z(m_rotate_z) * qad_delta / scale() / 80 );
        }else if(m_rotating){
            auto sizes = window().getSize();
            auto delta_pos = m_prev_mouse_pos - mouse_pos;
            m_rotate_z += delta_pos.x / sizes.x * M_PI;
            m_rotate_y += delta_pos.y / sizes.y * M_PI;
            // std::cout << m_rotate_x << " " << m_rotate_y << "\n";
        }
        m_prev_mouse_pos = mouse_pos;
    }
    else if (event.type() == sf::Event::MouseButtonReleased) {
        m_dragging = false;
        m_rotating = false;
    }
    else if (event.type() == sf::Event::KeyPressed) {
        // FIXME: This is too complex and doesn't even work for all cases.
        if (event.event().key.code == sf::Keyboard::Right) {
            if (m_speed > 0)
                m_speed *= 2;
            else {
                if (m_speed == 0)
                    m_speed = 1;
                else
                    m_speed /= 2;
            }
        }
        else if (event.event().key.code == sf::Keyboard::Left) {
            // if (m_speed < 0)
            //     m_speed *= 2;
            // else {
            //     if (m_speed == 0)
            //         m_speed = -1;
            //     else
            //         m_speed /= 2;
            // }

            if (m_speed > 0)
                m_speed /= 2;
        }
    }
    m_changed = m_dragging | m_rotating;
}

void SimulationView::draw_grid(sf::RenderWindow& window) const {
    constexpr float zoom_step_exponent = 2;
    auto zoom_rounded_to_power = std::pow(zoom_step_exponent, std::round(std::log2(m_zoom) / std::log2(zoom_step_exponent)));
    float spacing = 1 / zoom_rounded_to_power;
    {
        WorldDrawScope scope(*this);
        constexpr int major_gridline_interval = 4;
        float bounds = 500 * spacing;
        // Vector3 start_coords = screen_to_world({ 0, 0 });
        // start_coords.x -= std::remainder(start_coords.x, spacing * major_gridline_interval) + spacing;
        // start_coords.y -= std::remainder(start_coords.y, spacing * major_gridline_interval) + spacing;
        // Vector3 end_coords = screen_to_world({ size().x, size().y });
        Vector3 start_coords = { -bounds, -bounds, 0 };
        Vector3 end_coords = { bounds, bounds, 0 };

        if (start_coords.x > end_coords.x)
            std::swap(start_coords.x, end_coords.x);
        if (start_coords.y > end_coords.y)
            std::swap(start_coords.y, end_coords.y);

        sf::Color const major_grid_line_color { 60, 60, 60 };
        sf::Color const grid_line_color { 25, 25, 25 };

        // TODO: Create proper API for it.
        std::vector<sf::Vector3f> vertices;
        std::vector<sf::Vector3f> colors;

        int index = 0;

        // FIXME: Calculate bounds depending on window size instead of hardcoding them
        for (float x = start_coords.x; x < end_coords.x; x += spacing) {
            auto color = index % major_gridline_interval == 0 ? major_grid_line_color : grid_line_color;
            vertices.push_back({ x, -bounds, 0 });
            colors.push_back({ color.r / 255.f, color.g / 255.f, color.b / 255.f });
            vertices.push_back({ x, bounds, 0 });
            colors.push_back({ color.r / 255.f, color.g / 255.f, color.b / 255.f });
            index++;
        }
        index = 0;
        for (float y = start_coords.y; y < end_coords.y; y += spacing) {
            auto color = index % major_gridline_interval == 0 ? major_grid_line_color : grid_line_color;
            vertices.push_back({ -bounds, y, 0 });
            colors.push_back({ color.r / 255.f, color.g / 255.f, color.b / 255.f });
            vertices.push_back({ bounds, y, 0 });
            colors.push_back({ color.r / 255.f, color.g / 255.f, color.b / 255.f });
            index++;
        }

        glVertexPointer(3, GL_FLOAT, 0, vertices.data());
        glColorPointer(3, GL_FLOAT, 0, colors.data());
        glDrawArrays(GL_LINES, 0, vertices.size());
    }

    // guide
    sf::Vector2f guide_start { size().x - 200.f, size().y - 30.f };
    // HACK: this *100 should be calculated from perspective somehow
    sf::Vector2f guide_end = guide_start - sf::Vector2f(spacing * scale() * 100, 0);
    sf::VertexArray guide { sf::Lines, 6 };
    sf::Color const guide_color { 127, 127, 127 };
    guide[0] = sf::Vertex({ guide_start, guide_color });
    guide[1] = sf::Vertex({ guide_end, guide_color });
    guide[2] = sf::Vertex({ guide_start - sf::Vector2f(0, 5), guide_color });
    guide[3] = sf::Vertex({ guide_start + sf::Vector2f(0, 5), guide_color });
    guide[4] = sf::Vertex({ guide_end - sf::Vector2f(0, 5), guide_color });
    guide[5] = sf::Vertex({ guide_end + sf::Vector2f(0, 5), guide_color });
    window.draw(guide);

    // FIXME: UB on size_t conversion
    sf::Text text { std::to_string((size_t)(spacing * AU) / 1000) + " km", GUI::font, 15 };
    text.setPosition(guide_start);
    auto bounds = text.getLocalBounds();
    text.setOrigin({ bounds.width, bounds.height + 10 });
    window.draw(text);
}

Matrix4x4d SimulationView::projection_matrix() const {
    double fov = 80;
    double fd = 1 / std::tan(fov / 2);
    double aspect = size().x / (double)size().y;
    double zFar = 100;
    double zNear = 0.1;

    return { { { fd / aspect, 0, 0, 0 },
        { 0, fd, 0, 0 },
        { 0, 0, (zFar + zNear) / (zNear - zFar), -1 },
        { 0, 0, (2 * zFar * zNear) / (zNear - zFar), 0 } } };
}

Matrix4x4d SimulationView::modelview_matrix() const {
    Matrix4x4d matrix = Matrix4x4d::identity();
    matrix = matrix * Transform::translation(m_offset);
    matrix = matrix * Transform::scale({ scale(), scale(), scale() });
    matrix = matrix * Transform::rotation_around_x(0.7) * Transform::rotation_around_y(m_rotate_y) * Transform::rotation_around_z(m_rotate_z);
    matrix = matrix * Transform::translation({ 0, 0, -5 });
    return matrix;
}

Matrix4x4d SimulationView::rotation_matrix() const {
    Matrix4x4d x_rot =  { { { 1, 0, 0, 0 },
                            { 0, std::cos(m_rotate_x), -std::sin(m_rotate_x), 0 },
                            { 0, std::sin(m_rotate_x), -std::cos(m_rotate_x), 0 },
                            { 0, 0, 0, 0} } };

    Matrix4x4d y_rot =  { { { std::cos(m_rotate_y), 0, std::sin(m_rotate_y), 0 },
                            { 0, 1, 0, 0 },
                            { -std::sin(m_rotate_y), 0, std::cos(m_rotate_y), 0 },
                            { 0, 0, 0, 0 } } };

    Matrix4x4d z_rot =  { { { std::cos(m_rotate_z), -std::sin(m_rotate_z), 0, 0 },
                            { std::sin(m_rotate_z), -std::cos(m_rotate_z), 0, 0 },
                            { 0, 0, 1, 0 },
                            { 0, 0, 0, 0} } };

    return x_rot * y_rot * z_rot;
}

Vector3 SimulationView::screen_to_world(Vector3 v) const {
    // https://learnopengl.com/Getting-started/Coordinate-Systems
    // v = rotation_matrix().inverted() * v;
    Vector3 clip_space { -(v.x - size().x / 2.0) * 2 / size().x, (v.y - size().y / 2.0) * 2 / size().y };
    auto view_space = projection_matrix().inverted() * clip_space;
    // We skip world space because we have combined model+view matrix
    auto local_space = modelview_matrix().inverted() * view_space;
    // std::cout << "S 2 W " << v << " -> " << clip_space << " -> " << view_space << " -> " << local_space << std::endl;
    return local_space;
}

Vector3 SimulationView::world_to_screen(Vector3 local_space) const {
    // We skip world space because we have combined model+view matrix
    auto view_space = modelview_matrix() * local_space;
    auto clip_space = projection_matrix() * view_space;
    Vector3 result = { (clip_space.x + 1) / 2 * size().x, (-clip_space.y + 1) / 2 * size().y, 0 };
    // result = rotation_matrix() * result;
    // std::cout << "modelview: " << modelview_matrix() << ":: " << local_space << " -> " << view_space << " -> " << clip_space << "->" << result << std::endl;
    return result;
}

void SimulationView::draw(sf::RenderWindow& window) const {
    draw_grid(window);
    m_world.draw(*this);

    if (m_coord_measure) {
        auto sizes = size();
        sf::VertexArray lines(sf::Lines, 4);
        lines[0] = sf::Vertex { { 0, static_cast<float>(m_prev_mouse_pos.y) }, sf::Color::Red };
        lines[1] = sf::Vertex { { sizes.x, static_cast<float>(m_prev_mouse_pos.y) }, sf::Color::Red };
        lines[2] = sf::Vertex { { static_cast<float>(m_prev_mouse_pos.x), 0 }, sf::Color::Red };
        lines[3] = sf::Vertex { { static_cast<float>(m_prev_mouse_pos.x), sizes.y }, sf::Color::Red };
        window.draw(lines);
    }
    else if (m_focus_measure) {
        auto sizes = size();
        sf::VertexArray lines(sf::Lines, 4);
        lines[0] = sf::Vertex { { 0, static_cast<float>(m_prev_mouse_pos.y) }, sf::Color::Green };
        lines[1] = sf::Vertex { { sizes.x, static_cast<float>(m_prev_mouse_pos.y) }, sf::Color::Green };
        lines[2] = sf::Vertex { { static_cast<float>(m_prev_mouse_pos.x), 0 }, sf::Color::Green };
        lines[3] = sf::Vertex { { static_cast<float>(m_prev_mouse_pos.x), sizes.y }, sf::Color::Green };
        window.draw(lines);
    }

    std::ostringstream oss;
    oss << m_world.date.to_string();
    if (m_speed == 0)
        oss << " (Paused";
    else
        oss << " (" << std::abs(m_speed) << "x";

    if (m_speed < 0)
        oss << ", Reversed";
    oss << ")";

    sf::Text fps_text("FPS: " + std::to_string(m_fps), GUI::font, 25);
    fps_text.setFillColor(sf::Color::White);
    fps_text.setPosition(10, window.getSize().y - 65);
    window.draw(fps_text);

    sf::Text date_text(oss.str(), GUI::font, 25);
    date_text.setFillColor(sf::Color::White);
    date_text.setPosition(10, window.getSize().y - 35);
    window.draw(date_text);

    std::ostringstream debugoss;
    auto mp = sf::Mouse::getPosition(window);
    debugoss << "ws=" << screen_to_world({ static_cast<double>(mp.x), static_cast<double>(mp.y), 0 }) << std::endl;
    debugoss << "s=" << scale() << std::endl;
    debugoss << "off=" << offset() << std::endl;

    sf::Text debug_text(debugoss.str(), GUI::font, 15);
    debug_text.setPosition(500, 10);
    window.draw(debug_text);
}

void SimulationView::update() {
    // FIXME: This doesn't quite match here like speed (The same
    //        comment about Simulation object)
    if (m_speed != 0)
        m_world.update(m_speed * m_iterations);

    // Handle focus
    if (m_focused_object)
        set_offset(m_focused_object->m_pos);
}

WorldDrawScope::WorldDrawScope(SimulationView const& view)
    : m_simulation_view(view) {
    view.window().popGLStates();

    glViewport(0, 0, view.size().x, view.size().y);

    // simple OpenGL test
    glMatrixMode(GL_PROJECTION);
    view.projection_matrix().gl_load();

    glMatrixMode(GL_MODELVIEW);
    view.modelview_matrix().gl_load();

    glEnable(GL_DEPTH_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
}

WorldDrawScope::~WorldDrawScope() {
    glFlush();
    m_simulation_view.window().pushGLStates();
    m_simulation_view.window().resetGLStates();
}
