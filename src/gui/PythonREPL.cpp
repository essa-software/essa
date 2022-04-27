#include "PythonREPL.hpp"

#include "../pyssa/Environment.hpp"
#include "Application.hpp"
#include "ArrowButton.hpp"

namespace GUI {

PythonREPL::PythonREPL(WidgetTreeRoot& c)
    : Container(c) {
    auto& layout = set_layout<VerticalBoxLayout>();
    m_console = add_widget<Console>();
    m_textbox = add_widget<Textbox>();
    m_textbox->set_size({ { 100, Length::Percent }, 40.0_px });
    m_textbox->set_position({ 0.0_px, 0.0_px_o });
    m_textbox->set_data_type(Textbox::Type::TEXT);
    m_textbox->set_placeholder("PySSA Command");
}

void PythonREPL::handle_event(Event& event) {
    switch (event.type()) {
    case sf::Event::KeyPressed:
        if (!m_textbox->is_focused())
            break;
        if (event.event().key.code == sf::Keyboard::Enter) {
            auto content = m_textbox->get_content().toAnsiString();
            m_commands.push_back(content);
            m_curr_command = m_commands.size();
            content += "\n";
            m_console->append_line({ .color = sf::Color(100, 255, 255), .text = ">>> " + content });
            auto result = PySSA::Environment::the().eval_string(content);
            if (!result) {
                std::cout << "ERROR!!" << std::endl;

                auto message = PySSA::Environment::the().generate_exception_message();
                for (auto& line : message)
                    m_console->append_line({ .color = sf::Color(255, 100, 100), .text = line });
            }
            else {
                m_console->append_line({ .color = sf::Color(200, 200, 200), .text = result.repr() });
            }
            m_textbox->set_content("");
            event.set_handled();
        }
        else if (event.event().key.code == sf::Keyboard::Up) {
            if (m_curr_command > 0) {
                m_curr_command--;
                m_textbox->set_content(m_commands[m_curr_command]);
            }
        }
        else if (event.event().key.code == sf::Keyboard::Down) {
            if (m_commands.size() != 0 && m_curr_command < m_commands.size() - 1) {
                m_curr_command++;
                m_textbox->set_content(m_commands[m_curr_command]);
            }
            else {
                m_curr_command = m_commands.size();
                m_textbox->set_content("");
            }
        }
        else if (event.event().key.code == sf::Keyboard::L && event.event().key.control) {
            if (m_textbox->is_focused())
                m_console->clear();
        }
        break;
    default:
        break;
    }
}

}
