#pragma once

#include "Widget.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <string>

class Slider : public Widget {
    double m_val;
    const double m_min_val, m_max_val;

    double m_step;

    bool m_dragging = false;

    sf::Color m_bg_color, m_fg_color, m_text_color;
    unsigned m_text_size;
    std::string m_string;

public:
    enum class Mode {
        Linear,
        Exponential
    };

    enum class TextPos {
        TOP, RIGHT, BOTTOM, LEFT
    };

    Slider(GUI&, double min_val, double max_val, double step = 1);
    double get_value() const;
    double get_raw_value() const { return m_val; }
    void set_value(const double val);

    // NOTE: Mode affects only returned value and display, e.g min/max values
    //       are exponents.
    void set_mode(Mode mode) { m_mode = mode; }
    void set_exponent(double exp) { m_exponent = exp; }

    virtual void handle_event(sf::Event& event) override;
    virtual void draw(sf::RenderWindow& window) const override;

    void set_display_attributes(sf::Color bg_color, sf::Color fg_color);
    void set_text_attributes(unsigned text_size, std::string string, TextPos text_pos = TextPos::RIGHT);

private:
    float calculate_knob_size() const;

    TextPos m_text_pos;
    Mode m_mode = Mode::Linear;
    double m_exponent = 2;
};