#pragma once

#include <format>
#include <iostream>
#include <type_traits>

template <typename T>
concept FormattableType = requires { typename std::formatter<std::remove_cvref_t<T>, char>; };

template <FormattableType... Args>
void print_to(std::ostream& out, std::format_string<Args...> fmt, Args&&... xs) {
    out << std::format(fmt, std::forward<Args>(xs)...);
}

template <FormattableType... Args>
void println_to(std::ostream& out, std::format_string<Args...> fmt, Args&&... xs) {
    out << std::format(fmt, std::forward<Args>(xs)...) << '\n';
}

template <FormattableType... Args>
void print_to(std::fstream& out, std::format_string<Args...> fmt, Args&&... xs) {
    out << std::format(fmt, std::forward<Args>(xs)...);
}

template <FormattableType... Args>
void println_to(std::fstream& out, std::format_string<Args...> fmt, Args&&... xs) {
    out << std::format(fmt, std::forward<Args>(xs)...) << '\n';
}

template <FormattableType... Args>
void print(std::format_string<Args...> fmt, Args&&... xs) {
    print_to(std::cout, fmt, std::forward<Args>(xs)...);
}

template <FormattableType... Args>
void println(std::format_string<Args...> fmt, Args&&... xs) {
    println_to(std::cout, fmt, std::forward<Args>(xs)...);
}

template <FormattableType... Args>
void eprint(std::format_string<Args...> fmt, Args&&... xs) {
    print_to(std::cerr, fmt, std::forward<Args>(xs)...);
}

template <FormattableType... Args>
void eprintln(std::format_string<Args...> fmt, Args&&... xs) {
    println_to(std::cerr, fmt, std::forward<Args>(xs)...);
}
