/*
 * output.cpp - word-wrapped text output.
 */
#include "internal.h"

#include <ostream>

namespace adv::detail
{

void Output::out(std::string_view text)
{
    std::size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '\n') {
            *os_ << '\n';
            col_ = 0;
            ++i;
            continue;
        }
        if (text[i] == ' ') {
            ++i;
            continue;
        }
        std::size_t end = text.find_first_of(" \n", i);
        if (end == std::string_view::npos)
            end = text.size();
        const std::string_view word = text.substr(i, end - i);
        const int len = static_cast<int>(word.size());
        if (col_ > 0 && col_ + 1 + len > wrap_width) {
            *os_ << '\n';
            col_ = 0;
        } else if (col_ > 0) {
            *os_ << ' ';
            ++col_;
        }
        *os_ << word;
        col_ += len;
        i = end;
    }
}

void Output::say(std::string_view text)
{
    out(text);
    *os_ << '\n';
    col_ = 0;
}

void Output::raw(std::string_view text)
{
    *os_ << text;
}

} // namespace adv::detail
