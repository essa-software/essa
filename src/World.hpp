#pragma once

#include "Constants.hpp"
#include "Object.hpp"
#include "Vector2.hpp"
#include "gui/Date.hpp"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <cstdint>
#include <list>
#include <string>

class World{
private:
    sf::Event event;
    Object* focused_object = nullptr;

    // TODO: Move it to GUI.
    unsigned clicks = 0, speed = 1;
    Vector2 prev_pos;

public:
    explicit World(sf::RenderWindow& window);

    static sf::Font font;
    static Object* most_massive_object;
    View view;
    Date date;
    static unsigned object_count;
    static bool collisions, dragging, reverse;
    static std::list<Object> object_list;

    World();
    void get_events(sf::Event& event);
    void update();
    void draw();
    void add_object(const Object& object);
    void handle_focus();
    Object& get_object(const std::string name);
};

void prepare_solar(World& world);
