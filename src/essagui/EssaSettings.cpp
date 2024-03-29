#include "EssaSettings.hpp"

#include "../World.hpp"
#include "../glwrapper/Sphere.hpp"
#include <Essa/GUI/Widgets/Container.hpp>
#include <Essa/GUI/Widgets/StateTextButton.hpp>
#include <Essa/GUI/Widgets/TabWidget.hpp>
#include <Essa/GUI/Widgets/Textbox.hpp>
#include <Essa/GUI/Widgets/Textfield.hpp>
#include <Essa/GUI/Widgets/ValueSlider.hpp>
#include <EssaUtil/SimulationClock.hpp>

#include <fstream>

EssaSettings::EssaSettings(SimulationView& simulation_view)
    : m_simulation_view(simulation_view) {
}

void EssaSettings::on_init() {
    set_layout<GUI::VerticalBoxLayout>().set_spacing(10);

    auto title_label = add_widget<GUI::Textfield>();
    title_label->set_content("Settings");
    title_label->set_size({ Util::Length::Auto, 30.0_px });
    title_label->set_alignment(GUI::Align::Center);

    auto tab_widget = add_widget<GUI::TabWidget>();
    auto& simulation_settings = tab_widget->add_tab("Simulation");
    {
        auto& layout = simulation_settings.set_layout<GUI::VerticalBoxLayout>();
        layout.set_spacing(10);
        layout.set_padding(GUI::Boxi::all_equal(10));

        auto iterations_control = simulation_settings.add_widget<GUI::ValueSlider>();
        iterations_control->set_min(1);
        iterations_control->set_max(1000);
        iterations_control->set_name("Iterations");
        iterations_control->set_unit("i/t");
        m_on_restore_defaults.push_back([iterations_control]() {
            iterations_control->set_value(10);
        });
        iterations_control->set_tooltip_text("Count of simulation ticks per render frame");
        iterations_control->on_change = [this](double value) {
            if (value > 0)
                m_simulation_view.set_iterations(value);
        };
        iterations_control->set_class_name("Simulation");

        auto tick_length_control = simulation_settings.add_widget<GUI::ValueSlider>();
        tick_length_control->set_min(60);
        tick_length_control->set_max(60 * 60 * 24);
        tick_length_control->set_step(60);
        tick_length_control->set_name("Tick Length");
        tick_length_control->set_unit("s/t");
        m_on_restore_defaults.push_back([tick_length_control]() {
            tick_length_control->set_value(600); // 10 minutes
        });
        tick_length_control->set_tooltip_text("Amount of simulation seconds per simulation tick (Affects accuracy)");
        tick_length_control->on_change = [this](double value) {
            if (value > 0)
                m_simulation_view.world().set_simulation_seconds_per_tick(value);
        };

        auto reset_trails_button = simulation_settings.add_widget<GUI::TextButton>();
        reset_trails_button->set_size({ Util::Length::Auto, 30.0_px });
        reset_trails_button->set_content("Clear Trails");
        reset_trails_button->on_click = [&]() {
            m_simulation_view.world().reset_all_trails();
        };
    }

    auto add_toggle = [&](GUI::Container& container, Util::UString title, auto on_change, bool default_value = true) {
        auto toggle_container = container.add_widget<GUI::Container>();
        auto& toggle_layout = toggle_container->set_layout<GUI::HorizontalBoxLayout>();
        toggle_layout.set_spacing(10);
        toggle_container->set_size({ Util::Length::Auto, 30.0_px });
        auto button_label = toggle_container->add_widget<GUI::Textfield>();
        button_label->set_content(title + ": ");
        button_label->set_size({ { 70, Util::Length::Percent }, 30.0_px });
        auto toggle = toggle_container->add_widget<GUI::TextButton>();
        toggle->set_content("Off");
        toggle->set_active_content("On");
        toggle->set_toggleable(true);
        toggle->set_active(true);
        toggle->set_alignment(GUI::Align::Center);
        toggle->on_change = std::move(on_change);
        m_on_restore_defaults.push_back([toggle, default_value]() {
            toggle->set_active(default_value);
        });
        return toggle;
    };

    auto& display_settings = tab_widget->add_tab("Display");
    {
        auto& layout = display_settings.set_layout<GUI::VerticalBoxLayout>();
        layout.set_spacing(10);
        layout.set_padding(GUI::Boxi::all_equal(10));

        auto fov_control = display_settings.add_widget<GUI::ValueSlider>();
        fov_control->set_min(20);
        fov_control->set_max(160);
        fov_control->set_name("FOV");
        fov_control->set_unit("deg");
        m_on_restore_defaults.push_back([fov_control]() {
            fov_control->set_value(80);
        });
        fov_control->set_tooltip_text("Field of View");
        fov_control->on_change = [this](double value) {
            if (value > 0)
                m_simulation_view.set_fov(Util::Angle::degrees(static_cast<float>(value)));
        };

        auto toggle_sphere_mode_container = display_settings.add_widget<GUI::Container>();
        auto& toggle_sphere_mode_layout = toggle_sphere_mode_container->set_layout<GUI::HorizontalBoxLayout>();
        toggle_sphere_mode_layout.set_spacing(10);
        toggle_sphere_mode_container->set_size({ Util::Length::Auto, 30.0_px });

        auto sphere_mode_label = toggle_sphere_mode_container->add_widget<GUI::Textfield>();
        sphere_mode_label->set_content("Sphere mode: ");
        sphere_mode_label->set_size({ { 60, Util::Length::Percent }, Util::Length::Auto });

        auto toggle_sphere_mode = toggle_sphere_mode_container->add_widget<GUI::StateTextButton<Sphere::DrawMode>>();
        toggle_sphere_mode->add_state("Full", Sphere::DrawMode::Full, theme().positive);
        toggle_sphere_mode->add_state("Grid", Sphere::DrawMode::Grid, theme().negative);
        toggle_sphere_mode->add_state("Fancy", Sphere::DrawMode::Fancy, theme().neutral);
        toggle_sphere_mode->set_alignment(GUI::Align::Center);
        m_on_restore_defaults.push_back([toggle_sphere_mode]() {
            toggle_sphere_mode->set_index(2);
            toggle_sphere_mode->on_change(Sphere::DrawMode::Fancy);
        });
        toggle_sphere_mode->on_change = [](Sphere::DrawMode mode) {
            Object::sphere().set_draw_mode(mode);
        };

        auto toggle_time_format_container = display_settings.add_widget<GUI::Container>();
        auto& toggle_time_format_layout = toggle_time_format_container->set_layout<GUI::HorizontalBoxLayout>();
        toggle_time_format_layout.set_spacing(10);
        toggle_time_format_container->set_size({ Util::Length::Auto, 30.0_px });

        auto toggle_time_format_label = toggle_time_format_container->add_widget<GUI::Textfield>();
        toggle_time_format_label->set_content("Time format: ");
        toggle_time_format_label->set_size({ { 60, Util::Length::Percent }, Util::Length::Auto });

        auto toggle_time_format = toggle_time_format_container->add_widget<GUI::StateTextButton<Util::SimulationClock::Format>>();
        toggle_time_format->set_alignment(GUI::Align::Center);
        toggle_time_format->add_state("American", Util::SimulationClock::Format::AMERICAN, Util::Colors::Blue);
        toggle_time_format->add_state("Short", Util::SimulationClock::Format::SHORT_TIME, Util::Colors::Blue);
        toggle_time_format->add_state("Mid", Util::SimulationClock::Format::MID_TIME, Util::Colors::Blue);
        toggle_time_format->add_state("Long", Util::SimulationClock::Format::LONG_TIME, Util::Colors::Blue);
        toggle_time_format->add_state("No clock American", Util::SimulationClock::Format::NO_CLOCK_AMERICAN, Util::Colors::Blue);
        toggle_time_format->add_state("No clock short", Util::SimulationClock::Format::NO_CLOCK_SHORT, Util::Colors::Blue);
        toggle_time_format->add_state("No clock mid", Util::SimulationClock::Format::NO_CLOCK_MID, Util::Colors::Blue);
        toggle_time_format->add_state("No clock long", Util::SimulationClock::Format::NO_CLOCK_LONG, Util::Colors::Blue);

        m_on_restore_defaults.push_back([toggle_time_format]() {
            toggle_time_format->set_index(0);
        });

        toggle_time_format->on_change = [](Util::SimulationClock::Format state) {
            Util::SimulationClock::time_format = state;
        };

        add_toggle(display_settings, "Show labels", [this](bool state) {
            this->m_simulation_view.toggle_label_visibility(state);
        });
        add_toggle(display_settings, "Show grid", [this](bool state) {
            this->m_simulation_view.set_show_grid(state);
        });
        add_toggle(display_settings, "Show trails", [this](bool state) {
            this->m_simulation_view.set_show_trails(state);
        });
        add_toggle(display_settings, "Offset trails", [this](bool state) {
            this->m_simulation_view.set_offset_trails(state);
            this->m_simulation_view.world().reset_all_trails();
        });
        add_toggle(
            display_settings, "Display debug info", [this](bool state) {
                this->m_simulation_view.set_display_debug_info(state);
            },
            false);
    }

    auto& controls_settings = tab_widget->add_tab("Controls");
    {
        auto& layout = controls_settings.set_layout<GUI::VerticalBoxLayout>();
        layout.set_spacing(10);
        layout.set_padding(GUI::Boxi::all_equal(10));

        add_toggle(controls_settings, "Pause when adding object", [this](bool state) {
            m_pause_simulation_on_creative_mode = state;
        });

        add_toggle(
            controls_settings, "Fix rotation on focused object", [this](bool state) {
                m_simulation_view.set_fixed_rotation_on_focus(state);
            },
            false);

        add_toggle(
            controls_settings, "Unfocus when object window closed", [this](bool state) {
                m_unfocus_on_wnd_close = state;
            },
            false);
    }

    auto restore_container = add_widget<GUI::Container>();
    auto& restore_layout = restore_container->set_layout<GUI::HorizontalBoxLayout>();
    restore_layout.set_spacing(10);
    restore_container->set_size({ Util::Length::Auto, 30.0_px });

    auto restore_sim = restore_container->add_widget<GUI::TextButton>();
    restore_sim->set_content("Restore simulation");
    restore_sim->set_toggleable(false);
    restore_sim->set_alignment(GUI::Align::Center);
    restore_sim->override_button_colors().normal.set_colors(theme().neutral);
    restore_sim->set_tooltip_text("Restore Simulation to state from the beginning");
    restore_sim->on_click = [this]() {
        reset_simulation();
    };

    auto restore_defaults = restore_container->add_widget<GUI::TextButton>();
    restore_defaults->set_content("Restore defaults");
    restore_defaults->set_toggleable(false);
    restore_defaults->set_alignment(GUI::Align::Center);
    restore_defaults->override_button_colors().normal.set_colors(theme().neutral);

    restore_defaults->on_click = [this]() {
        for (auto& func : m_on_restore_defaults)
            func();
    };
    restore_defaults->on_click();
}

void EssaSettings::reset_simulation() {
    m_simulation_view.world().reset(m_world_file);
    m_simulation_view.reset();
}

void EssaSettings::load_world(std::string wf) {
    set_world_file(std::move(wf));
    reset_simulation();
}
