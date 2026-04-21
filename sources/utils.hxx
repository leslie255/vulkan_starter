#pragma once

#include <format>
#include <iostream>
#include <source_location>
#include <string_view>
#include <type_traits>

#include <vulkan/vulkan.h>

void assert_vk_success(
    VkResult result,
    std::source_location location = std::source_location::current());

/// Similar to C++32's `std::ranges::enumerate_view`.
/// Turns iterator into [index, value] pairs.
template <
    typename T,
    typename TIter = decltype(std::begin(std::declval<T>())),
    typename = decltype(std::end(std::declval<T>()))>
constexpr auto enumerate(T&& iterable) {
    struct iterator {
        size_t i;
        TIter iter;
        bool operator!=(const iterator& other) const {
            return iter != other.iter;
        }
        void operator++() {
            ++i;
            ++iter;
        }
        auto operator*() const {
            return std::tie(i, *iter);
        }
    };
    struct iterable_wrapper {
        T iterable;
        auto begin() {
            return iterator {0, std::begin(iterable)};
        }
        auto end() {
            return iterator {0, std::end(iterable)};
        }
    };
    return iterable_wrapper {std::forward<T>(iterable)};
}

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
