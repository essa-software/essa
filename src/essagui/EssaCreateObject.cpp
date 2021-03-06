#include "EssaCreateObject.hpp"

#include "EssaGUI.hpp"

#include <EssaGUI/gui/MessageBox.hpp>
#include <EssaGUI/gui/ValueSlider.hpp>
#include <EssaUtil/Units.hpp>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

EssaCreateObject::EssaCreateObject(GUI::Container& c, SimulationView& simulation_view)
    : GUI::Container(c)
    , m_simulation_view(simulation_view) {
    set_layout<GUI::VerticalBoxLayout>().set_spacing(10);

    m_create_object_gui(*this);

    auto mode_specific_options_container = add_widget<Container>();
    mode_specific_options_container->set_size({ Length::Auto, 150.0_px });
    mode_specific_options_container->set_layout<GUI::BasicLayout>();

    m_create_object_from_params_container = m_create_object_from_params_gui(*mode_specific_options_container);
    m_create_object_from_params_container->set_size({ { 100, Length::Percent }, { 100, Length::Percent } });
    m_create_object_from_params_container->set_visible(true);
    mode_specific_options_container->add_created_widget(m_create_object_from_params_container);

    m_create_object_from_orbit_container = m_create_object_from_orbit_gui(*mode_specific_options_container);
    m_create_object_from_orbit_container->set_size({ { 100, Length::Percent }, { 100, Length::Percent } });
    m_create_object_from_orbit_container->set_visible(false);
    mode_specific_options_container->add_created_widget(m_create_object_from_orbit_container);

    m_create_name_and_color_container();

    auto mode_specific_submit_container = add_widget<Container>();
    mode_specific_submit_container->set_size({ Length::Auto, 72.0_px });
    mode_specific_submit_container->set_layout<GUI::BasicLayout>();
    m_toggle_unit_button = m_create_toggle_unit_button(*mode_specific_submit_container);

    m_submit_create_container = m_create_submit_container(*mode_specific_submit_container);
    m_submit_create_container->set_size({ { 100, Length::Percent }, { 100, Length::Percent } });
    m_submit_create_container->set_visible(true);
    m_submit_create_container->add_created_widget(m_toggle_unit_button);
    mode_specific_submit_container->add_created_widget(m_submit_create_container);
}

void EssaCreateObject::m_create_name_and_color_container() {
    auto main_color_container = add_widget<Container>();
    main_color_container->set_size({ Length::Auto, 150.0_px });
    auto& main_color_layout = main_color_container->set_layout<GUI::VerticalBoxLayout>();
    main_color_layout.set_spacing(10);
    {
        auto color_label_textfield = main_color_container->add_widget<GUI::Textfield>();
        color_label_textfield->set_content("COLOR");
        color_label_textfield->set_alignment(GUI::Align::Center);

        m_color_control = main_color_container->add_widget<GUI::ColorPicker>();
        m_color_control->set_size({ Length::Auto, 100.0_px });
        m_color_control->on_change = [this](auto) {
            m_forward_simulation_is_valid = false;
        };
    }
    auto name_container = add_widget<Container>();
    auto& name_layout = name_container->set_layout<GUI::HorizontalBoxLayout>();
    name_layout.set_spacing(10);
    {
        auto name_textfield = name_container->add_widget<GUI::Textfield>();
        name_textfield->set_size({ 150.0_px, Length::Auto });
        name_textfield->set_content("Name: ");
        name_textfield->set_alignment(GUI::Align::CenterLeft);

        m_name_textbox = name_container->add_widget<GUI::Textbox>();
        m_name_textbox->set_limit(20);
        m_name_textbox->set_data_type(GUI::Textbox::TEXT);
        m_name_textbox->set_content("Planet");
        m_name_textbox->on_change = [this](auto) {
            m_forward_simulation_is_valid = false;
        };
    }
}

std::shared_ptr<GUI::Container> EssaCreateObject::m_create_submit_container(GUI::Container& parent) {
    auto container = std::make_shared<GUI::Container>(parent);
    container->set_size({ Length::Auto, 72.0_px });
    auto& submit_layout = container->set_layout<GUI::HorizontalBoxLayout>();
    submit_layout.set_spacing(10);
    {
        m_coords_button = container->add_widget<GUI::ImageButton>(resource_manager().require_texture("coordsButton.png"));
        m_coords_button->on_click = [this]() {
            if (m_automatic_orbit_calculation) {
                m_simulation_view.start_focus_measure([&](Object* focusing) {
                    m_add_object_button->set_visible(true);
                    m_focused = focusing;
                    m_forward_simulation_is_valid = false;
                });
            }
            else {
                m_simulation_view.start_coords_measure([&](Util::Vector3d pos) {
                    m_add_object_button->set_visible(true);
                    m_forward_simulation_is_valid = false;
                    m_new_object_pos = pos;
                });
            }
        };
        m_coords_button->set_tooltip_text("Set position");

        auto creative_mode_button = container->add_widget<GUI::ImageButton>(resource_manager().require_texture("toggleCreativeModeButton.png"));
        creative_mode_button->set_toggleable(true);
        creative_mode_button->set_tooltip_text("Toggle automatic orbit calculation");

        creative_mode_button->on_change = [this](bool state) {
            m_create_object_from_params_container->set_visible(!state);
            m_create_object_from_orbit_container->set_visible(state);
            m_toggle_orbit_direction_button->set_visible(state);
            m_require_orbit_point_button->set_visible(!state);
            m_automatic_orbit_calculation = state;
        };

        m_toggle_orbit_direction_button = container->add_widget<GUI::ImageButton>(resource_manager().require_texture("orbitDirectionButton.png"));
        m_toggle_orbit_direction_button->set_toggleable(true);
        m_toggle_orbit_direction_button->set_tooltip_text("Toggle orbitting body direction");
        m_toggle_orbit_direction_button->on_change = [this](bool state) {
            m_forward_simulation_is_valid = false;
        };
        m_toggle_orbit_direction_button->set_visible(false);

        m_require_orbit_point_button = container->add_widget<GUI::ImageButton>(resource_manager().require_texture("requireOrbitPointButton.png"));
        m_require_orbit_point_button->set_tooltip_text("Recalculate apoapsis/periapsis so that the orbit passes the point");
        m_require_orbit_point_button->on_click = [this]() {
            if (!m_new_object) {
                GUI::Application::the().spawn_notification("You need to specify initial coords of the object", GUI::Application::NotificationLevel::Error);
                return;
            }
            m_simulation_view.start_coords_measure([this](Util::Vector3d coords) {
                assert(m_new_object);
                m_new_object->require_orbit_point(coords);
            });
        };

        container->add_widget<Widget>(); // spacer

        m_add_object_button = container->add_widget<GUI::ImageButton>(resource_manager().require_texture("addObjectButton.png"));
        m_add_object_button->on_click = [this]() {
            if (m_simulation_view.world().exist_object_with_name(m_name_textbox->get_content().encode())) {
                GUI::message_box("Object with name: \"" + m_name_textbox->get_content() + "\" already exist!", "Error!", GUI::MessageBox::Buttons::Ok);
                return;
            }

            if (m_automatic_orbit_calculation)
                m_simulation_view.world().add_object(m_create_object_from_orbit());
            else
                m_simulation_view.world().add_object(m_create_object_from_params());
            m_simulation_view.m_measured = false;
        };
    }

    return container;
}

void EssaCreateObject::recalculate_forward_simulation() {
    if (m_to_modify != nullptr)
        return;

    if (m_automatic_orbit_calculation)
        m_new_object = m_create_object_from_orbit();
    else
        m_new_object = m_create_object_from_params();

    if (!m_new_object)
        return;

    m_simulation_view.world().clone_for_forward_simulation(m_forward_simulated_world);

    // We need trail of the forward simulated object but
    // the object itself will be drawn at current position.
    auto forward_simulated_new_object = m_new_object->clone_for_forward_simulation();
    m_forward_simulated_new_object = forward_simulated_new_object.get();
    m_forward_simulated_world.add_object(std::move(forward_simulated_new_object));

    m_forward_simulated_world.update(m_forward_simulation_ticks_control->value());

    m_forward_simulation_is_valid = true;
}

std::shared_ptr<GUI::ImageButton> EssaCreateObject::m_create_toggle_unit_button(GUI::Container& parent) {
    auto button = std::make_shared<GUI::ImageButton>(parent, resource_manager().require_texture("toggleUnitButton.png"));
    button->set_toggleable(true);
    button->on_change = [this](bool state) {
        m_prev_unit_state = state;

        auto vel = m_velocity_control->value();
        auto dist = find_widgets_of_type_by_class_name_recursively<GUI::ValueSlider>("Dist");
        auto angle = find_widgets_of_type_by_class_name_recursively<GUI::ValueSlider>("Angle");

        if (state) {
            this->m_velocity_control->set_unit("km/h");
            this->m_velocity_control->set_value(vel * 3.6);
            this->m_velocity_control->slider().set_range(0, 500000, 1);

            for (auto& d : dist) {
                d->set_unit("AU");
                d->set_value(d->value() / AU * 1000);
                d->slider().set_range(0, 100, 0.1);
            }
            this->m_y_position_control->slider().set_range(-50, 50, 0.1);

            for (auto& a : angle) {
                a->set_unit("[rad]");
                a->set_value(a->value() / 180 * M_PI);
                a->slider().set_range(0, 2 * M_PI, 0.01);
            }
            this->m_orbit_tilt_control->slider().set_range(-M_PI / 2, M_PI / 2, 0.01);
            this->m_direction_yz_control->slider().set_range(-M_PI / 2, M_PI / 2, 0.01);
        }
        else {
            this->m_velocity_control->set_unit("m/s");
            this->m_velocity_control->set_value(vel / 3.6);
            this->m_velocity_control->slider().set_range(0, 500000 / 3.6, 1);

            for (auto& d : dist) {
                d->set_unit("km");
                d->set_value(d->value() / 1000 * AU);
                d->slider().set_range(0, 0.1 * AU, 0.1);
            }
            this->m_y_position_control->slider().set_range(-50 * AU, 50 * AU, 100);

            for (auto& a : angle) {
                a->set_unit("[deg]");
                a->set_value(a->value() / M_PI * 180);
                a->slider().set_range(0, 360, 1);
            }
            this->m_orbit_tilt_control->slider().set_range(-90, 90, 1);
            this->m_direction_yz_control->slider().set_range(-90, 90, 1);
        }
    };
    button->set_tooltip_text("Toggle units");

    return button;
}

void EssaCreateObject::m_create_object_gui(GUI::Container& container) {
    auto label = container.add_widget<GUI::Textfield>();
    label->set_content("Create Object");
    label->set_size({ Length::Auto, 30.0_px });
    label->set_alignment(GUI::Align::Center);

    m_forward_simulation_ticks_control = container.add_widget<GUI::ValueSlider>(1, 1000);
    m_forward_simulation_ticks_control->set_name("Simulate");
    m_forward_simulation_ticks_control->set_unit("ticks");
    m_forward_simulation_ticks_control->set_tooltip_text("How many ticks should be forward-simulated");
    m_forward_simulation_ticks_control->on_change = [this](auto) {
        m_forward_simulation_is_valid = false;
    };

    m_radius_control = container.add_widget<GUI::ValueSlider>(0, 500000);
    m_radius_control->set_name("Radius");
    m_radius_control->set_unit("km");
    m_radius_control->on_change = [this](auto) {
        m_forward_simulation_is_valid = false;
    };

    auto mass_container = container.add_widget<GUI::Container>();
    {
        auto& mass_layout = mass_container->set_layout<GUI::HorizontalBoxLayout>();
        mass_layout.set_spacing(10);

        auto mass_textfield = mass_container->add_widget<GUI::Textfield>();
        mass_textfield->set_size({ 150.0_px, Length::Auto });
        mass_textfield->set_content("Mass: ");
        mass_textfield->set_alignment(GUI::Align::CenterLeft);

        m_mass_textbox = mass_container->add_widget<GUI::Textbox>();
        m_mass_textbox->set_limit(6);
        m_mass_textbox->set_content("1.0");
        m_mass_textbox->set_min_max_values(1, 9.9);
        m_mass_textbox->on_change = [this](auto) {
            m_forward_simulation_is_valid = false;
        };

        auto mass_value_container = mass_container->add_widget<GUI::Container>();
        mass_value_container->set_layout<GUI::HorizontalBoxLayout>();
        {
            auto mass_exponent_textfield = mass_value_container->add_widget<GUI::Textfield>();
            mass_exponent_textfield->set_content(" * 10 ^ ");
            mass_exponent_textfield->set_alignment(GUI::Align::CenterLeft);

            m_mass_exponent_textbox = mass_value_container->add_widget<GUI::Textbox>();
            m_mass_exponent_textbox->set_limit(2);
            m_mass_exponent_textbox->set_content("1");
            m_mass_exponent_textbox->set_min_max_values(1, 40);
            m_mass_exponent_textbox->on_change = [this](auto) {
                m_forward_simulation_is_valid = false;
            };

            auto mass_unit_textfield = mass_value_container->add_widget<GUI::Textfield>();
            mass_unit_textfield->set_content(" kg ");
            mass_unit_textfield->set_alignment(GUI::Align::Center);
        }
        mass_layout.set_multipliers({ 5.f / 3, 5.f / 3, 5.f / 9, 5.f / 9, 5.f / 9 });
    }
}

std::shared_ptr<GUI::Container> EssaCreateObject::m_create_object_from_params_gui(GUI::Container& parent) {
    auto container = std::make_shared<GUI::Container>(parent);
    container->set_layout<GUI::VerticalBoxLayout>().set_spacing(5);

    m_velocity_control = container->add_widget<GUI::ValueSlider>(0, 50000);
    m_velocity_control->set_name("Velocity");
    m_velocity_control->set_unit("m/s");
    m_velocity_control->on_change = [this](double) {
        m_forward_simulation_is_valid = false;
    };

    m_direction_xz_control = container->add_widget<GUI::ValueSlider>(0, 360, 1);
    m_direction_xz_control->set_name("Direction X");
    m_direction_xz_control->set_unit("[deg]");
    m_direction_xz_control->set_class_name("Util::Angle");
    m_direction_xz_control->slider().set_wraparound(true);
    m_direction_xz_control->on_change = [this](double) {
        m_forward_simulation_is_valid = false;
    };

    m_direction_yz_control = container->add_widget<GUI::ValueSlider>(-90, 90, 1);
    m_direction_yz_control->set_name("Direction Y");
    m_direction_yz_control->set_unit("[deg]");
    m_direction_yz_control->set_class_name("Util::Angle");
    m_direction_yz_control->slider().set_wraparound(true);
    m_direction_yz_control->on_change = [this](double) {
        m_forward_simulation_is_valid = false;
    };

    m_y_position_control = container->add_widget<GUI::ValueSlider>(-0.05 * AU, 0.05 * AU);
    m_y_position_control->set_name("Y position");
    m_y_position_control->set_unit("km");
    m_y_position_control->set_class_name("Dist");
    m_y_position_control->on_change = [this](double) {
        m_forward_simulation_is_valid = false;
    };

    return container;
}

std::shared_ptr<GUI::Container> EssaCreateObject::m_create_object_from_orbit_gui(GUI::Container& parent) {
    auto container = std::make_shared<GUI::Container>(parent);
    container->set_layout<GUI::VerticalBoxLayout>().set_spacing(5);

    m_apogee_control = container->add_widget<GUI::ValueSlider>(0, 0.05 * AU);
    m_apogee_control->set_name("Apogee");
    m_apogee_control->set_unit("km");
    m_apogee_control->set_class_name("Dist");
    m_apogee_control->on_change = [this](double) {
        m_forward_simulation_is_valid = false;
    };

    m_perigee_control = container->add_widget<GUI::ValueSlider>(0, 0.05 * AU);
    m_perigee_control->set_name("Perigee");
    m_perigee_control->set_unit("km");
    m_perigee_control->set_class_name("Dist");
    m_perigee_control->on_change = [this](double) {
        m_forward_simulation_is_valid = false;
    };

    m_orbit_angle_control = container->add_widget<GUI::ValueSlider>(0, 360, 1);
    m_orbit_angle_control->set_name("Util::Angle");
    m_orbit_angle_control->set_unit("[deg]");
    m_orbit_angle_control->set_class_name("Util::Angle");
    m_orbit_angle_control->slider().set_wraparound(true);
    m_orbit_angle_control->on_change = [this](double) {
        m_forward_simulation_is_valid = false;
    };

    m_orbit_tilt_control = container->add_widget<GUI::ValueSlider>(-90, 90, 1);
    m_orbit_tilt_control->set_name("Tilt");
    m_orbit_tilt_control->set_unit("[deg]");
    m_orbit_tilt_control->set_class_name("Util::Angle");
    m_orbit_tilt_control->slider().set_wraparound(true);
    m_orbit_tilt_control->on_change = [this](double) {
        m_forward_simulation_is_valid = false;
    };

    return container;
}

std::unique_ptr<Object> EssaCreateObject::m_create_object_from_params() const {
    double mass = std::stod(m_mass_textbox->get_content().encode()) * std::pow(10, std::stod(m_mass_exponent_textbox->get_content().encode()));
    double radius = m_radius_control->value() * 1000;
    double theta = m_direction_xz_control->value();
    double alpha = m_direction_yz_control->value();
    double velocity = m_velocity_control->value();

    if (!m_toggle_unit_button->is_active()) {
        theta = theta / 180 * M_PI;
        alpha = alpha / 180 * M_PI;
    }

    Util::Vector3d vel(std::cos(theta) * std::cos(alpha) * velocity, std::sin(theta) * std::cos(alpha) * velocity, std::sin(alpha) * velocity);

    auto pos = m_new_object_pos;
    pos.z() = m_y_position_control->value();

    Util::Color color = m_color_control->value();
    auto name = m_name_textbox->get_content().encode();
    return std::make_unique<Object>(mass, radius, pos, vel, color, name, 1000);
}

std::unique_ptr<Object> EssaCreateObject::m_create_object_from_orbit() const {
    if (!m_focused)
        return nullptr;
    double mass = std::stod(m_mass_textbox->get_content().encode()) * std::pow(10, std::stod(m_mass_exponent_textbox->get_content().encode()));
    Distance radius = { static_cast<float>(m_radius_control->value() * 1000), Distance::Kilometer };
    Distance apogee, perigee;
    Util::Angle angle, tilt;

    if (m_toggle_unit_button->is_active()) {
        apogee = { static_cast<float>(m_apogee_control->value() * AU), Distance::Au };
        perigee = { static_cast<float>(m_perigee_control->value() * AU), Distance::Au };

        angle = { static_cast<float>(m_orbit_angle_control->value()), Util::Angle::Rad };
        tilt = { static_cast<float>(m_orbit_tilt_control->value()), Util::Angle::Rad };
    }
    else {
        apogee = { static_cast<float>(m_apogee_control->value() * 1000), Distance::Kilometer };
        perigee = { static_cast<float>(m_perigee_control->value() * 1000), Distance::Kilometer };

        angle = { static_cast<float>(m_orbit_angle_control->value()), Util::Angle::Deg };
        tilt = { static_cast<float>(m_orbit_tilt_control->value()), Util::Angle::Deg };
    }

    return m_focused->create_object_relative_to_ap_pe(mass, radius, apogee, perigee, m_toggle_orbit_direction_button->is_active(), angle, tilt, m_color_control->value(), m_name_textbox->get_content().encode(), 0.0_deg);
}
