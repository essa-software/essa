#include "Slider.hpp"
#include "../Vector2.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <cmath>
#include <iostream>

Slider::Slider(GUI& gui, double min_val, double max_val, double step)
: Widget(gui), m_min_val(min_val), m_max_val(max_val), m_step(step), m_mode(0), m_val((min_val + max_val) / 2)
{
}

double Slider::get_value() const
{
    return m_val;
}

double& Slider::set_value(const double val)
{
    if(val < m_min_val)
        m_val = m_min_val;
    else if(val > m_max_val)
        m_val = m_max_val;
    else
        m_val = val;
    return m_val;
}

void Slider::set_display_attributes(sf::Color bg_color, sf::Color fg_color)
{
    m_bg_color = bg_color;
    m_fg_color = fg_color;
}

void Slider::set_text_attributes(unsigned text_size, std::string string, TextPos text_pos)
{
    m_text_size = text_size;
    m_string = string;
    m_text_pos = text_pos;
}

Vector2 mouse_pos;

void Slider::handle_event(sf::Event& event)
{
    float posx, posy;
    if(m_mode)
        posx = position().x + std::log10(m_val - m_min_val) / std::log10(m_max_val - m_min_val) * size().x;
    else
        posx = position().x + (m_val - m_min_val) / (m_max_val - m_min_val) * size().y;
    posy = position().y - size().x * 2;

    if(event.type == sf::Event::MouseButtonPressed)
    {
        if(is_mouse_over({event.mouseButton.x, event.mouseButton.y}))
            m_dragging = true;
    }
    else if(event.type == sf::Event::MouseButtonReleased)
    {
        m_dragging = false;
    }
    else if(sf::Event::MouseMoved)
    {
        if(m_dragging)
        {
            auto mouse_pos_relative_to_slider = sf::Vector2f({static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y)}) - position();
            m_val = (mouse_pos_relative_to_slider.x / size().x) * (m_max_val - m_min_val) + m_min_val;
            m_val = std::min(std::max(m_min_val, m_val), m_max_val);

            // round to step
            m_val /= m_step;
            m_val = std::floor(m_val);
            m_val *= m_step;
        }
    }
}

float Slider::calculate_knob_size() const
{
    return std::max(4.0, size().x / (m_max_val - m_min_val) * m_step);
}

void Slider::draw(sf::RenderWindow& window) const
{
    sf::RectangleShape slider({size().x, 5.f});
    slider.setPosition(position().x, position().y + size().y / 2 - 2.5f);
    slider.setFillColor(m_bg_color);
    window.draw(slider);

    // std::cout << slider.getPosition().x << " " << slider.getPosition().y << "\n";

    sf::RectangleShape bound;
    bound.setSize(sf::Vector2f(2, 10));
    bound.setFillColor(m_bg_color);
    bound.setPosition(position().x, position().y + size().y / 2 - 5);
    window.draw(bound);
    bound.setPosition(position().x + size().x - 2, position().y + size().y / 2 - 5);
    window.draw(bound);

    sf::RectangleShape slider_value;
    auto knob_size_x = calculate_knob_size();
    slider_value.setSize(sf::Vector2f(knob_size_x, size().y));
    slider_value.setFillColor(m_fg_color);

    if(m_mode)
        slider_value.setPosition(position().x + std::log10(m_val - m_min_val) / std::log10(m_max_val - m_min_val) * size().y - knob_size_x / 2, position().y);
    else
        slider_value.setPosition(position().x + (m_val - m_min_val) / (m_max_val - m_min_val) * size().x - knob_size_x / 2, position().y);
    window.draw(slider_value);
    // std::cout << "XD\n";
}
