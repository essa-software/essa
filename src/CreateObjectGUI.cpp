#include "EssaGUI.hpp"
#include <memory>

#include <iostream>

void EssaGUI::m_create_object_gui(GUI::Container& container) {
    auto label = container.add_widget<GUI::Textfield>();
    label->set_content("Create Object");
    label->set_size({ Length::Auto, 30.0_px });
    label->set_alignment(GUI::Align::Center);

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

std::shared_ptr<GUI::Container> EssaGUI::m_create_object_from_params_gui(std::shared_ptr<GUI::Container> parent) {
    auto container = std::make_shared<GUI::Container>(*parent);
    container->set_layout<GUI::VerticalBoxLayout>().set_spacing(5);

    m_velocity_control = container->add_widget<GUI::ValueSlider>(0, 500000);
    m_velocity_control->set_name("Velocity");
    m_velocity_control->set_unit("m/s");
    m_velocity_control->on_change = [this](double) {
        m_forward_simulation_is_valid = false;
    };

    m_direction_control = container->add_widget<GUI::ValueSlider>(0, 360, 0.1);
    m_direction_control->set_name("Direction");
    m_direction_control->set_unit("[deg]");
    m_direction_control->slider().set_wraparound(true);
    m_direction_control->on_change = [this](double) {
        m_forward_simulation_is_valid = false;
    };
    return container;
}

std::shared_ptr<GUI::Container> EssaGUI::m_create_object_from_orbit_gui(std::shared_ptr<GUI::Container> parent) {
    auto container = std::make_shared<GUI::Container>(*parent);
    container->set_layout<GUI::VerticalBoxLayout>().set_spacing(5);

    // FIXME: This should use ValueSlider.
    // FIXME: Add on_change callback to invalidate forward simulation.
    semi_major_axis_container = container->add_widget<GUI::Container>();
    auto& semi_major_axis_layout = semi_major_axis_container->set_layout<GUI::HorizontalBoxLayout>();
    semi_major_axis_layout.set_spacing(10);
    {
        auto semi_major_axis_textfield = semi_major_axis_container->add_widget<GUI::Textfield>();
        semi_major_axis_textfield->set_size({ 150.0_px, Length::Auto });
        semi_major_axis_textfield->set_content("Semi major axis magnitude: ");
        semi_major_axis_textfield->set_alignment(GUI::Align::CenterLeft);

        m_semi_major_axis_textbox = semi_major_axis_container->add_widget<GUI::Textbox>();
        m_semi_major_axis_textbox->set_limit(20);
        m_semi_major_axis_textbox->set_data_type(GUI::Textbox::NUMBER);
        m_semi_major_axis_textbox->set_content("0");

        auto semi_major_axis_unit_textfield = semi_major_axis_container->add_widget<GUI::Textfield>();
        semi_major_axis_unit_textfield->set_size({ 150.0_px, Length::Auto });
        semi_major_axis_unit_textfield->set_content("km");
        semi_major_axis_unit_textfield->set_alignment(GUI::Align::CenterRight);
    }
    semi_major_axis_layout.set_multipliers({ 1.5, 1, 0.5 });

    semi_minor_axis_container = container->add_widget<GUI::Container>();
    auto& semi_minor_axis_layout = semi_minor_axis_container->set_layout<GUI::HorizontalBoxLayout>();
    semi_minor_axis_layout.set_spacing(10);
    {
        auto semi_minor_axis_textfield = semi_minor_axis_container->add_widget<GUI::Textfield>();
        semi_minor_axis_textfield->set_size({ 150.0_px, Length::Auto });
        semi_minor_axis_textfield->set_content("Semi minor axis magnitude: ");
        semi_minor_axis_textfield->set_alignment(GUI::Align::CenterLeft);

        m_semi_minor_axis_textbox = semi_minor_axis_container->add_widget<GUI::Textbox>();
        m_semi_minor_axis_textbox->set_limit(20);
        m_semi_minor_axis_textbox->set_data_type(GUI::Textbox::NUMBER);
        m_semi_minor_axis_textbox->set_content("0");

        auto semi_minor_axis_unit_textfield = semi_minor_axis_container->add_widget<GUI::Textfield>();
        semi_minor_axis_unit_textfield->set_size({ 150.0_px, Length::Auto });
        semi_minor_axis_unit_textfield->set_content("km");
        semi_minor_axis_unit_textfield->set_alignment(GUI::Align::CenterRight);
    }
    semi_minor_axis_layout.set_multipliers({ 1.5, 1, 0.5 });
    return container;
}

std::unique_ptr<Object> EssaGUI::m_create_object_from_params() const {
    double mass = std::stod(m_mass_textbox->get_content().toAnsiString()) * std::pow(10, std::stod(m_mass_exponent_textbox->get_content().toAnsiString()));
    double radius = m_radius_control->value() * 1000;

    double theta = m_direction_control->value() / 360 * 2 * M_PI;
    double velocity = m_velocity_control->value();
    if (m_units)
        velocity /= 3.6;
    Vector3 vel(std::cos(theta) * velocity, std::sin(theta) * velocity);

    sf::Color color = m_color_control->value();
    // std::cout << (int)color.r << "," << (int)color.g << "," << (int)color.b << "," << (int)color.a << std::endl;

    std::string name = m_name_textbox->get_content();

    return std::make_unique<Object>(m_world, mass, radius, m_new_object_pos, vel, color, name, 1000);
}