#pragma once

#include <expected>
#include <variant>

// https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// https://stackoverflow.com/questions/47203255/convert-stdvariant-to-another-stdvariant-with-super-set-of-types
template<class... Args>
struct variant_cast_proxy
{
    std::variant<Args...> v;

    template<class... ToArgs>
    operator std::variant<ToArgs...>() const
    {
        return std::visit(
            [](auto&& arg) -> std::variant<ToArgs...> { return arg; }, v);
    }
};

template<class... Args>
auto variant_cast(const std::variant<Args...>& v) -> variant_cast_proxy<Args...>
{
    return { v };
}

#define PROPAGATE(...)                                                         \
    ({                                                                         \
        auto err = __VA_ARGS__;                                                \
        if (!err)                                                              \
        {                                                                      \
            return std::unexpected(variant_cast(err.error()));                 \
        }                                                                      \
        std::move(err.value());                                                \
    })

#if __has_include(<experimental/scope>)
#include <experimental/scope>
template<typename T>
using ScopeExit = std::experimental::scope_exit<T>;
#else

template<typename T>
class ScopeExit
{
public:
    ScopeExit(T func) : m_func(std::forward<T>(func)) {}
    ~ScopeExit() { m_func(); }

private:
    T m_func;
};
#endif
