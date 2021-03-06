#pragma once

#include "Object.hpp"

#include <istream>
#include <memory>
#include <optional>

class ConfigLoader
{
public:
    explicit ConfigLoader(std::istream& in)
    : m_in(in) {}

    void load(World& world);

private:
    bool parse_statement(World& world);
    std::optional<std::pair<std::string, std::string>> parse_key_value_pair(std::istream& in);

    std::istream& m_in;
};
