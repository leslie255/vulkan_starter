#pragma once

#include <format>
#include <iostream>
#include <type_traits>

template <typename T>
concept FormattableType = requires { typename std::formatter<std::remove_cvref_t<T>, char>; };

template <FormattableType... Args>
void write(std::ostream& out, std::format_string<Args...> fmt, Args&&... xs) {
    out << std::format(fmt, std::forward<Args>(xs)...);
}

template <FormattableType... Args>
void writeln(std::ostream& out, std::format_string<Args...> fmt, Args&&... xs) {
    out << std::format(fmt, std::forward<Args>(xs)...) << '\n';
}

template <FormattableType... Args>
void print(std::format_string<Args...> fmt, Args&&... xs) {
    write(std::cout, fmt, std::forward<Args>(xs)...);
}

template <FormattableType... Args>
void println(std::format_string<Args...> fmt, Args&&... xs) {
    writeln(std::cout, fmt, std::forward<Args>(xs)...);
}

template <FormattableType... Args>
void eprint(std::format_string<Args...> fmt, Args&&... xs) {
    write(std::cerr, fmt, std::forward<Args>(xs)...);
}

template <FormattableType... Args>
void eprintln(std::format_string<Args...> fmt, Args&&... xs) {
    writeln(std::cerr, fmt, std::forward<Args>(xs)...);
}
