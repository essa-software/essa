#include "GUI.hpp"
#include "FreeButton.hpp"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <iostream>

GUI::GUI(sf::RenderWindow& window) : m_window(window), m_slider_1(0, 100, 1), m_slider_2(0, 100, 1), m_slider_3(0, 100, 1){
    sf::Image image;
    image.loadFromFile("../assets/createButton.png");
    FreeButton buttonCreate(sf::Vector2f(0, 0), image, 4, 0.4);
    m_create_button = buttonCreate;

    m_slider_1.set_display_attributes(sf::Color(200, 200, 200), sf::Color(255, 255, 255));
    m_slider_2.set_display_attributes(sf::Color(200, 200, 200), sf::Color(255, 255, 255));
    m_slider_3.set_display_attributes(sf::Color(200, 200, 200), sf::Color(255, 255, 255));

    m_slider_1.set_position(sf::Vector2f(50, 100));
    m_slider_2.set_position(sf::Vector2f(50, 200));
    m_slider_3.set_position(sf::Vector2f(50, 300));

    m_slider_1.set_size(200, 3);
    m_slider_2.set_size(200, 3);
    m_slider_3.set_size(200, 3);
}

void GUI::draw(){
    m_create_button.draw(m_window);

    if(m_create_button.m_mode >= 2){
        m_slider_1.draw(m_window);
        m_slider_2.draw(m_window);
        m_slider_3.draw(m_window);
    }
}

void GUI::get_events(sf::Event &event){
    m_create_button.event_listener(event, EventListening::ONCLICK);
    m_create_button.event_listener(event, EventListening::ONRELASE);

    if(m_create_button.m_mode >= 2){
        m_slider_1.get_events(event);
        m_slider_2.get_events(event);
        m_slider_3.get_events(event);
    }
}
