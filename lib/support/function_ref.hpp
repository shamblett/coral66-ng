/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <functional>
#include <memory>
#include <type_traits>

#include "utilities.hpp"

/**
 * FunctionRef is a non-owning reference to a callable object.
 *
 * It is a lightweight alternative to `std::function` that does not allocate memory. It is intended
 * for passing callbacks to functions where the callback is not stored beyond the scope of the call.
 * It type-erases the callable, allowing any compatible function, lambda, or functor to be passed.
 *
 * Key Features:
 * - No heap allocation.
 * - Cheap to copy (pointer-sized).
 * - Type erasure: Can hold any callable with a compatible signature.
 * - Non-owning: The caller must ensure the callable outlives the `FunctionRef`.
 */

namespace coral66
{
template <class Fn>
class FunctionRef;

template <class Ret, class... Params>
class FunctionRef<Ret(Params...)>
{
    Ret (*m_callback)(void*, Params... params){};
    void* m_callable{};

public:
    FunctionRef() = default;
    FunctionRef(std::nullptr_t) noexcept {}

    template <class Callable>
    requires(!std::is_same_v<std::remove_cvref_t<Callable>, FunctionRef> &&
             std::is_invocable_r_v<Ret, Callable, Params...>)
    explicit FunctionRef(Callable&& callable) noexcept
        : m_callback(+[](void* callable1, Params... params) -> Ret
                     {
                         return std::invoke_r<Ret>(
                             *static_cast<std::remove_reference_t<Callable>*>(callable1),
                             std::forward<Params>(params)...);
                     }),
          m_callable(const_cast<void*>(static_cast<const void*>(std::addressof(callable))))
    {
    }

    template <class... Args>
    Ret operator()(Args&&... args) const
    {
        CORAL66_ASSERT(m_callable);
        return m_callback(m_callable, std::forward<Args>(args)...);
    }

    explicit operator bool() const
    {
        return m_callable;
    }
};
} // namespace coral66
