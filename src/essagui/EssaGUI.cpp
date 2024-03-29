#include "EssaGUI.hpp"

#include "EssaSettings.hpp"
#include "FocusedObjectGUI.hpp"
#include "PythonREPL.hpp"
#include "SimulationInfo.hpp"

#include <Essa/GUI/Application.hpp>
#include <Essa/GUI/EML/EMLResource.hpp>
#include <Essa/GUI/Overlays/FilePrompt.hpp>
#include <Essa/GUI/Overlays/MessageBox.hpp>
#include <Essa/GUI/Overlays/ToolWindow.hpp>
#include <Essa/GUI/Widgets/MDI/Host.hpp>
#include <Essa/GUI/Widgets/NotificationContainer.hpp>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

EssaGUI::EssaGUI(World& world)
    : m_world(world) { }

void EssaGUI::on_init() {
    set_layout<GUI::BasicLayout>();

    m_mdi_host = add_widget<GUI::MDI::Host>();
    m_mdi_host->set_size({ 100_perc, 100_perc });

    {
        auto& root_container = m_mdi_host->set_background_widget<GUI::Container>();
        root_container.set_layout<GUI::BasicLayout>();

        m_simulation_view = root_container.add_widget<SimulationView>(m_world);
        m_simulation_view->set_size({ { 100, Util::Length::Percent }, { 100, Util::Length::Percent } });
        // m_simulation_view->set_visible(false);
        m_world.m_simulation_view = m_simulation_view;

        m_simulation_view->on_change_focus = [&](Object* obj) {
            if (obj == nullptr)
                return;

            auto focused_object_window = m_mdi_host->open_or_focus_window(Util::UString { obj->name() }, "FocusedGUI");
            if (focused_object_window.opened) {
                focused_object_window.window->set_position({ raw_size().x() - 550, 50 });
                focused_object_window.window->set_size({ 500, 600 });
                auto& focused_object_gui = focused_object_window.window->set_main_widget<FocusedObjectGUI>(obj, focused_object_window.window, m_world);
                focused_object_gui.update_params();
                focused_object_window.window->on_close = [&]() {
                    if (m_settings_gui->unfocus_on_wnd_close()) {
                        m_simulation_view->set_focused_object(nullptr);
                    }

                    if (m_simulation_view->is_paused()) {
                        m_simulation_view->pop_pause();
                    }
                };
            }
        };

        m_world.on_reset = [&]() {
            m_mdi_host->for_each_window([](GUI::MDI::Window& wnd) {
                if (wnd.id() == "FocusedGUI")
                    wnd.close();
            });
        };

        auto home_button = root_container.add_widget<GUI::ImageButton>();
        home_button->set_image(&resource_manager().require_texture("homeButton.png"));
        home_button->set_position({ 10.0_px, 10.0_px });
        home_button->set_alignments(GUI::Widget::Alignment::End, GUI::Widget::Alignment::End);
        home_button->on_click = [this]() {
            m_simulation_view->reset();
        };
        home_button->set_tooltip_text("Reset coordinates");

        auto menu = root_container.add_widget<GUI::SettingsMenu>();
        menu->set_position({ 10.0_px, 10.0_px });
        menu->set_size({ 100_perc, 100_perc });
        {
            auto& create_menu = menu->add_entry(resource_manager().require_texture("createButton.png"), "Create new object");
            create_menu.on_toggle = [this](bool state) {
                if (m_settings_gui->pause_simulation_on_creative_mode()) {
                    m_create_object_gui->set_new_object(nullptr);
                    m_create_object_gui->forward_simulation_state(true);
                    if (state) {
                        m_simulation_view->push_pause();
                    }
                    else {
                        m_simulation_view->pop_pause();
                    }
                }
                m_draw_forward_simulation = state;
            };
            create_menu.settings_container->set_layout<GUI::HorizontalBoxLayout>();
            create_menu.settings_container->set_size({ 500.0_px, 530.0_px });
            m_create_object_gui = create_menu.settings_container->add_widget<EssaCreateObject>(*m_simulation_view);
        }

        {
            auto& load_world = menu->add_entry(resource_manager().require_texture("loadWorld.png"), "Load world", GUI::SettingsMenu::Expandable::No);
            load_world.on_toggle = [this](bool) {
                auto prompt = GUI::Application::the().open_host_window<GUI::FilePrompt>("Choose file to open: ", "Open file", "e.g.: solar.essa");
                prompt.root.add_desired_extension(".essa");
                prompt.window.show_modal(&this->host_window());

                if (prompt.root.result().has_value())
                    m_settings_gui->load_world(prompt.root.result().value().encode());
            };
        }

        {
            auto& settings = menu->add_entry(resource_manager().require_texture("simulationSettings.png"), "Simulation Settings");
            settings.settings_container->set_layout<GUI::HorizontalBoxLayout>();
            settings.settings_container->set_size({ 500.0_px, 600.0_px });
            m_settings_gui = settings.settings_container->add_widget<EssaSettings>(*m_simulation_view);
        }

        {
            auto& pyssa = menu->add_entry(resource_manager().require_texture("python.png"), "PySSA (Python scripting)", GUI::SettingsMenu::Expandable::No);
            pyssa.on_toggle = [this](bool) {
                open_python_repl();
            };
        }

        auto fps_counter = root_container.add_widget<SimulationInfo>(m_simulation_view);
        fps_counter->set_size({ 600.0_px, 50.0_px });
        fps_counter->set_position({ 10.0_px, 10.0_px });
        fps_counter->set_vertical_alignment(GUI::Widget::Alignment::End);
    }

    auto notification_container = add_widget<GUI::NotificationContainer>();
    notification_container->set_size({ 500_px, 100_perc });
    notification_container->set_position({ Util::Length(host_window().size().x() / 2 - 250, Util::Length::Px), 10_px });
    m_notification_container = notification_container;

    host_window().on_event = [notification_container](llgl::Event const& event) -> GUI::Widget::EventHandlerResult {
        if (auto* resize = event.get<llgl::Event::WindowResize>()) {
            notification_container->set_position({ Util::Length(resize->new_size().x() / 2 - 250, Util::Length::Px), 10_px });
        }
        return GUI::Widget::EventHandlerResult::NotAccepted;
    };
}

void EssaGUI::open_python_repl() {
#ifdef ENABLE_PYSSA
    auto python_repl_window = m_mdi_host->open_window();
    python_repl_window.window.set_title("PySSA");
    python_repl_window.window.set_position({ 600, 750 });
    python_repl_window.window.set_size({ 700, 250 });
    m_mdi_host->focus_window(python_repl_window.window);
    python_repl_window.root.set_main_widget<PythonREPL>();
#else
    GUI::message_box("PySSA is not enabled on this build. Use Linux build with -DENABLE_PYSSA=1 instead.", "Error", GUI::MessageBox::Buttons::Ok);
#endif
}

void EssaGUI::update() {
    if (!m_create_object_gui->is_forward_simulation_valid())
        m_create_object_gui->recalculate_forward_simulation();
}

void EssaGUI::draw(Gfx::Painter& painter) const {
    if (m_create_object_gui->new_object() && m_draw_forward_simulation) {
        {
            GUI::WorldDrawScope scope(painter);
            m_create_object_gui->new_object()->draw(painter, *m_simulation_view);
            m_create_object_gui->forward_simulated_new_object()->draw_closest_approaches(painter, *m_simulation_view);
        }
        // FIXME: This should be drawn above grid.
        m_create_object_gui->forward_simulated_new_object()->draw_closest_approaches_gui(painter, *m_simulation_view);
        m_create_object_gui->new_object()->draw_gui(painter, *m_simulation_view);
        m_create_object_gui->forward_simulated_world().draw(painter, *m_simulation_view);
    }
}

GUI::Widget::EventHandlerResult EssaGUI::on_key_press(GUI::Event::KeyPress const& event) {
    if (event.code() == llgl::KeyCode::Tilde) {
        open_python_repl();
    }
    return GUI::Widget::EventHandlerResult::NotAccepted;
}
