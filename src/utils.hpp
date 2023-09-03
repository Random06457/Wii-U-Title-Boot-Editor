#pragma once

#if __has_include(<expected>)
#include <expected>
#ifdef __cpp_lib_expected
#define Expected std::expected
#define Unexpected std::unexpected
#else
#include <tl/expected.hpp>
#define Expected tl::expected
#define Unexpected tl::unexpected
#endif
#else
#include <tl/expected.hpp>
#define Expected tl::expected
#define Unexpected tl::unexpected
#endif
#include <variant>

template<typename T, typename... Errors>
using Result = Expected<T, std::variant<Errors...>>;

template<typename... Errors>
using Error = Expected<void, std::variant<Errors...>>;

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

#define PROPAGATE_VOID(...)                                                    \
    ({                                                                         \
        auto err = __VA_ARGS__;                                                \
        if (!err)                                                              \
        {                                                                      \
            return Unexpected(variant_cast(err.error()));                      \
        }                                                                      \
    })

#define PROPAGATE(...)                                                         \
    ({                                                                         \
        auto err = __VA_ARGS__;                                                \
        if (!err)                                                              \
        {                                                                      \
            return Unexpected(variant_cast(err.error()));                      \
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

#ifdef __cpp_lib_byteswap
#define byteswap std::byteswap
#else
template<typename T>
T byteswap(const T& v)
{
    static_assert(std::is_integral_v<T>);

    if constexpr (sizeof(T) == 2)
        return __builtin_bswap16(v);
    if constexpr (sizeof(T) == 4)
        return __builtin_bswap32(v);
    if constexpr (sizeof(T) == 8)
        return __builtin_bswap64(v);
    else
        std::abort();
}
#endif
