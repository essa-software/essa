#include "World.hpp"

#include "ConfigLoader.hpp"

#include <fstream>

void prepare_solar(World& world) {
    // std::ifstream config_file("../solar.essa");
    std::ifstream config_file("../2body.essa");
    ConfigLoader loader(config_file);
    loader.load(world);
}
