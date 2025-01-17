/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <LibCore/DateTime.h>
#include <LibCore/Timer.h>
#include <LibGUI/Application.h>
#include <LibGUI/Painter.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>
#include <LibGfx/Font.h>
#include <LibGfx/Palette.h>
#include <serenity.h>
#include <spawn.h>
#include <stdio.h>
#include <time.h>

class ClockWidget final : public GUI::Widget {
    C_OBJECT(ClockWidget)
public:
    ClockWidget()
    {
        m_time_width = Gfx::Font::default_bold_font().width("2222-22-22 22:22:22");

        m_timer = add<Core::Timer>(1000, [this] {
            static time_t last_update_time;
            time_t now = time(nullptr);
            if (now != last_update_time) {
                tick_clock();
                last_update_time = now;
            }
        });
    }

    virtual ~ClockWidget() override { }

    int get_width()
    {
        return m_time_width + menubar_menu_margin();
    }

private:
    static int menubar_menu_margin() { return 2; }

    virtual void paint_event(GUI::PaintEvent& event) override
    {
        auto time_text = Core::DateTime::now().to_string();
        GUI::Painter painter(*this);
        painter.fill_rect(event.rect(), palette().window());
        painter.draw_text(event.rect(), time_text, Gfx::Font::default_font(), Gfx::TextAlignment::Center, palette().window_text());
    }

    virtual void mousedown_event(GUI::MouseEvent& event) override
    {
        if (event.button() != GUI::MouseButton::Left)
            return;

        pid_t pid;
        const char* argv[] = { "Calendar", nullptr };
        if ((errno = posix_spawn(&pid, "/bin/Calendar", nullptr, nullptr, const_cast<char**>(argv), environ))) {
            perror("posix_spawn");
        } else {
            if (disown(pid) < 0)
                perror("disown");
        }
    }

    void tick_clock()
    {
        update();
    }

    RefPtr<Core::Timer> m_timer;
    int m_time_width;
};

int main(int argc, char** argv)
{
    if (pledge("stdio shared_buffer accept rpath unix cpath fattr exec proc", nullptr) < 0) {
        perror("pledge");
        return 1;
    }

    auto app = GUI::Application::construct(argc, argv);

    if (pledge("stdio shared_buffer accept rpath exec proc", nullptr) < 0) {
        perror("pledge");
        return 1;
    }

    auto window = GUI::Window::construct();
    window->set_title("Clock");
    window->set_window_type(GUI::WindowType::MenuApplet);

    auto& widget = window->set_main_widget<ClockWidget>();
    window->resize(widget.get_width(), 16);
    window->show();

    if (unveil("/res", "r") < 0) {
        perror("unveil");
        return 1;
    }

    if (unveil("/bin/Calendar", "x") < 0) {
        perror("unveil");
        return 1;
    }

    unveil(nullptr, nullptr);

    return app->exec();
}
