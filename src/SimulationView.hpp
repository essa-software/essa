#pragma once

#include "pyssa/WrappedObject.hpp"

#include <Essa/Engine/3D/Shaders/Basic.hpp>
#include <Essa/GUI/NotifyUser.hpp>
#include <Essa/GUI/Widgets/Widget.hpp>
#include <Essa/GUI/Widgets/WorldView.hpp>
#include <EssaUtil/Angle.hpp>
#include <EssaUtil/Constants.hpp>
#include <EssaUtil/Matrix.hpp>
#include <EssaUtil/Vector.hpp>

#include <functional>
#include <optional>

class Object;
class World;

class SimulationView : public GUI::WorldView
    , public PySSA::WrappedObject<SimulationView> {
public:
    explicit SimulationView(World& world);

    void set_offset(Util::DeprecatedVector3d o) { m_offset = o; }
    void set_zoom(double d) { m_zoom = d; }
    Util::DeprecatedVector3d offset() const { return m_offset; }
    double scale() const { return m_zoom; }
    void apply_zoom(double v) { m_zoom *= v; }
    void reset_rotation() {
        m_pitch = -0.7;
        m_pitch_from_object = 0;
        m_yaw = 0;
        m_yaw_from_object = 0;
    }

    double real_yaw() const { return m_yaw + m_yaw_from_object; }
    double real_pitch() const { return m_pitch + m_pitch_from_object; }

    std::optional<Util::DeprecatedVector3d> screen_to_world_on_grid(Util::DeprecatedVector2f screen) const;

    // The Z coordinate is in the clip space. So if you want to do clipping,
    // you need to check if z is in range <-1, 1>.
    Util::DeprecatedVector3f world_to_screen(Util::DeprecatedVector3d v) const;

    Util::DeprecatedVector3d screen_to_clip_space(Util::DeprecatedVector2f) const;
    Util::DeprecatedVector2f clip_space_to_screen(Util::DeprecatedVector3d) const;

    llgl::Camera camera() const;
    llgl::Projection projection() const;
    Util::Matrix4x4d matrix() const;

    llgl::Renderer& renderer() const;

    void reset() {
        m_offset = Util::DeprecatedVector3d { 0, 0, 0 };
        m_zoom = 1;
        m_focused_object = nullptr;
        reset_rotation();
    };

    void start_coords_measure(std::function<void(Util::DeprecatedVector3d)> handler) {
        m_measure = Measure::Coords;
        m_on_coord_measure = std::move(handler);
    }

    void start_focus_measure(std::function<void(Object*)> handler) {
        m_measure = Measure::Focus;
        m_on_focus_measure = std::move(handler);
    }

    std::function<void(Object*)> on_change_focus;
    std::function<void()> if_focused;

    void toggle_label_visibility(bool visibility) { m_show_labels = visibility; }
    bool show_labels() const { return m_show_labels; }
    void set_show_grid(bool b) { m_show_grid = b; }
    void set_show_trails(bool b) { m_show_trails = b; }
    bool show_trails() const { return m_show_trails; }
    void set_offset_trails(bool b) { m_offset_trail = b; }
    bool offset_trails() const { return m_offset_trail; }
    void set_fixed_rotation_on_focus(bool b) { m_fixed_rotation_on_focus = b; }
    void set_display_debug_info(bool b) { m_display_debug_info = b; }

    void set_fov(Util::Angle fov) { m_fov = fov; }

    // TODO: This should be private
    bool m_measured = false;

    // FIXME: This should be in some Simulation object.
    int speed() const { return is_paused() ? 0 : m_speed; }
    int raw_speed() const { return m_speed; }
    void set_speed(int speed) { m_speed = speed; }

    int iterations() const { return m_iterations; }
    void set_iterations(int i) { m_iterations = i; }

    World& world() { return m_world; }
    Object* focused_object() const;
    void set_focused_object(Object* obj, GUI::NotifyUser notify_user = GUI::NotifyUser::No);

    bool is_paused() const { return m_pause_count > 0; }
    void push_pause();
    void pop_pause();

    Gfx::FullShaderResource<Essa::Shaders::Basic>& basic_shader() const { return *m_basic_shader; }

#ifdef ENABLE_PYSSA
    static void setup_python_bindings(TypeSetup);
#endif

    static constexpr char const* PythonClassName = "SimulationView";

private:
    virtual EventHandlerResult on_mouse_button_press(GUI::Event::MouseButtonPress const&) override;
    virtual EventHandlerResult on_mouse_button_release(GUI::Event::MouseButtonRelease const&) override;
    virtual EventHandlerResult on_mouse_scroll(GUI::Event::MouseScroll const&) override;
    virtual EventHandlerResult on_mouse_move(GUI::Event::MouseMove const&) override;
    virtual EventHandlerResult on_key_press(GUI::Event::KeyPress const&) override;
    virtual void draw(Gfx::Painter&) const override;
    virtual void update() override;

    virtual bool accepts_focus() const override { return true; }

    void draw_grid(Gfx::Painter&) const;

#ifdef ENABLE_PYSSA
    PySSA::Object python_reset(PySSA::Object const& args, PySSA::Object const& kwargs);

    PySSA::Object python_get_offset() const;
    bool python_set_offset(PySSA::Object const&);
    PySSA::Object python_get_fov() const;
    bool python_set_fov(PySSA::Object const&);
    PySSA::Object python_get_yaw() const;
    bool python_set_yaw(PySSA::Object const&);
    PySSA::Object python_get_pitch() const;
    bool python_set_pitch(PySSA::Object const&);
    PySSA::Object python_get_world() const;
    // No setter for world
    PySSA::Object python_get_zoom() const;
    bool python_set_zoom(PySSA::Object const&);
    PySSA::Object python_get_focused_object() const;
    bool python_set_focused_object(PySSA::Object const&);
#endif

    Util::DeprecatedVector3d m_offset;
    Util::Angle m_fov = 80.0_deg;

    double m_yaw = 0;
    double m_pitch = 0;
    double m_yaw_from_object = 0;
    double m_pitch_from_object = 0;

    World& m_world;
    double m_zoom = 1;
    Object* m_focused_object = nullptr;
    Util::DeprecatedVector2f m_prev_mouse_pos;
    Util::DeprecatedVector2f m_prev_drag_pos;

    std::function<void(Util::DeprecatedVector3d pos)> m_on_coord_measure;
    std::function<void(Object* focused)> m_on_focus_measure;

    enum class DragMode {
        None,
        Pan,    // left click
        Rotate, // right click
    };
    DragMode m_drag_mode {};
    bool m_is_dragging { false };

    enum class Measure {
        None,
        Coords,
        Focus
    };

    Measure m_measure = Measure::None;

    bool m_show_labels = true;
    bool m_show_grid = true;
    bool m_show_trails = true;
    bool m_offset_trail = true;
    bool m_fixed_rotation_on_focus = true;
    bool m_display_debug_info = false;

    int m_iterations = 1;

    // FIXME: This doesn't quite match here (and also World). Maybe
    //        add some Simulation class.
    int m_speed = 1;
    int m_saved_speed = 1;
    int m_pause_count = 0;

    Gfx::FullShaderResource<Essa::Shaders::Basic>* m_basic_shader = nullptr;
};
