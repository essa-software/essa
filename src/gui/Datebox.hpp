#pragma once
#include "../util/SimulationClock.hpp"
#include "Container.hpp"
#include "StateTextButton.hpp"
#include "TextButton.hpp"
#include "Textfield.hpp"
#include <SFML/Graphics/Color.hpp>
#include <memory>
#include <vector>

namespace GUI {

class Datebox : public Container {
    Util::SimulationClock::time_point m_date;
    Textfield* m_date_textfield = nullptr;
    Textfield* m_century_textfield = nullptr;
    Textfield* m_year_textfield = nullptr;
    Textfield* m_month_textfield = nullptr;
    Container* m_calendar_container = nullptr;
    Container* m_final_row = nullptr;
    StateTextButton<bool>* m_toggle_container_button = nullptr;
    std::vector<TextButton*> m_calendar_contents;

    void m_create_container();
    TextButton* m_create_calendar_button(Container& c) const;
    void m_update_calendar();

public:
    explicit Datebox(Container& parent);

    void set_display_attributes(sf::Color bg_color, sf::Color fg_color, sf::Color text_color);
};

}