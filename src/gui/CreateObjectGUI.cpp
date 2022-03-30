#include "GUI.hpp"
#include <memory>

#include <iostream>

void GUI::m_create_object_gui(std::shared_ptr<Container> container){
    m_radius_control = container->add_widget<ValueSlider>(0, 500000);
    m_radius_control->set_name("Radius");
    m_radius_control->set_unit("km");

    auto mass_container = container->add_widget<Container>();
        {
        auto& mass_layout = mass_container->set_layout<HorizontalBoxLayout>();
        mass_layout.set_spacing(10);

        auto mass_textfield = mass_container->add_widget<Textfield>();
        mass_textfield->set_size({ 150.0_px, Length::Auto });
        mass_textfield->set_display_attributes(sf::Color(0, 0, 0), sf::Color(0, 0, 255), sf::Color(255, 255, 255));
        mass_textfield->set_font_size(20);
        mass_textfield->set_content("Mass: ");
        mass_textfield->set_alignment(Textfield::Align::CenterLeft);

        m_mass_textbox = mass_container->add_widget<Textbox>();
        m_mass_textbox->set_display_attributes(sf::Color(255, 255, 255), sf::Color(200, 200, 200), sf::Color(150, 150, 150));
        m_mass_textbox->set_limit(6);
        m_mass_textbox->set_content("1.0");

        auto mass_value_container = mass_container->add_widget<Container>();
        mass_value_container->set_layout<HorizontalBoxLayout>();
        {
            auto mass_exponent_textfield = mass_value_container->add_widget<Textfield>();
            mass_exponent_textfield->set_display_attributes(sf::Color(0, 0, 0), sf::Color(0, 0, 255), sf::Color(255, 255, 255));
            mass_exponent_textfield->set_font_size(20);
            mass_exponent_textfield->set_content(" * 10 ^ ");
            mass_exponent_textfield->set_alignment(Textfield::Align::CenterLeft);
            
            m_mass_exponent_textbox = mass_value_container->add_widget<Textbox>();
            m_mass_exponent_textbox->set_display_attributes(sf::Color(255, 255, 255), sf::Color(200, 200, 200), sf::Color(150, 150, 150));
            m_mass_exponent_textbox->set_limit(2);
            m_mass_exponent_textbox->set_content("1");

            auto mass_unit_textfield = mass_value_container->add_widget<Textfield>();
            mass_unit_textfield->set_display_attributes(sf::Color(0, 0, 0), sf::Color(0, 0, 255), sf::Color(255, 255, 255));
            mass_unit_textfield->set_font_size(20);
            mass_unit_textfield->set_content(" kg ");
            mass_unit_textfield->set_alignment(Textfield::Align::Center);
        }
        mass_layout.set_multipliers({ 5.f / 3, 5.f / 3, 5.f / 9, 5.f / 9, 5.f / 9 });
    }
}

void GUI::m_create_object_from_params_gui(std::shared_ptr<Container> container, bool visible) {
    m_velocity_control = container->add_widget<ValueSlider>(0, 500000);
    m_velocity_control->set_name("Velocity");
    m_velocity_control->set_unit("m/s");
    m_velocity_control->set_visible(visible);

    m_direction_control = container->add_widget<ValueSlider>(0, 360, 0.1);
    m_direction_control->set_name("Direction");
    m_direction_control->set_unit("[deg]");
    m_direction_control->slider().set_wraparound(true);
    m_direction_control->set_visible(visible);
}

std::unique_ptr<Object> GUI::m_create_object_from_params() const {
    double mass = std::stod(m_mass_textbox->get_content().toAnsiString()) * std::pow(10, std::stod(m_mass_exponent_textbox->get_content().toAnsiString()));
    double radius = m_radius_control->value() * 1000;

    double theta = m_direction_control->value() / 360 * 2 * M_PI;
    double velocity = m_velocity_control->value();
    Vector2 vel(std::cos(theta) * velocity, std::sin(theta) * velocity);

    sf::Color color = m_color_control->value();
    // std::cout << (int)color.r << "," << (int)color.g << "," << (int)color.b << "," << (int)color.a << std::endl;

    std::string name = m_name_textbox->get_content();

    return std::make_unique<Object>((Object(m_world, mass, radius, m_new_object_pos, vel, color, name, 1000)));
}
