#include "PythonREPL.hpp"

#include "../pyssa/Environment.hpp"
#include <Essa/GUI/Application.hpp>
#include <Essa/GUI/Widgets/ArrowButton.hpp>

void PythonREPL::on_init() {
    set_layout<GUI::VerticalBoxLayout>();
    m_console = add_widget<GUI::Console>();
    m_textbox = add_widget<GUI::Textbox>();
    m_textbox->set_position({ 0.0_px, {} });
    m_textbox->set_vertical_alignment(GUI::Widget::Alignment::End);
    m_textbox->set_type(GUI::Textbox::Type::TEXT);
    m_textbox->set_placeholder("PySSA Command");
    m_textbox->set_focused();
}

GUI::Widget::EventHandlerResult PythonREPL::on_key_press(GUI::Event::KeyPress const& event) {
    if (!m_textbox->is_focused())
        return GUI::Widget::EventHandlerResult::NotAccepted;
    if (event.code() == llgl::KeyCode::Enter) {
        auto content = m_textbox->content();
        m_commands.push_back(content);
        m_curr_command = m_commands.size();
        content = content + "\n";
        m_console->append_line({ .color = Util::Color { 100, 255, 255 }, .text = ">>> " + content });
        auto result = PySSA::Environment::the().eval_string(content.encode());
        if (!result) {
            std::cout << "ERROR!!" << std::endl;

            auto message = PySSA::Environment::the().generate_exception_message();
            for (auto& line : message)
                m_console->append_line({ .color = Util::Color { 255, 100, 100 }, .text = Util::UString { line } });
        }
        else {
            m_console->append_line({ .color = Util::Color { 200, 200, 200 }, .text = Util::UString { result.repr() } });
        }
        m_textbox->set_content("");
        return GUI::Widget::EventHandlerResult::Accepted;
    }
    else if (event.code() == llgl::KeyCode::Up) {
        std::cout << "test Up" << std::endl;
        if (m_curr_command > 0) {
            m_curr_command--;
            m_textbox->set_content(m_commands[m_curr_command]);
        }
    }
    else if (event.code() == llgl::KeyCode::Down) {
        std::cout << "test Down" << std::endl;
        if (m_commands.size() != 0 && m_curr_command < m_commands.size() - 1) {
            m_curr_command++;
            m_textbox->set_content(m_commands[m_curr_command]);
        }
        else {
            m_curr_command = m_commands.size();
            m_textbox->set_content("");
        }
    }
    else if (event.code() == llgl::KeyCode::L && event.modifiers().ctrl) {
        if (m_textbox->is_focused())
            m_console->clear();
    }
    return GUI::Widget::EventHandlerResult::NotAccepted;
}
