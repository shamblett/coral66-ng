/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

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

    template <class Callable>
    requires(!std::is_same_v<std::decay_t<Callable>, FunctionRef>) explicit FunctionRef(Callable&& callable)
        : m_callback(+[](void* callable1, Params... params) -> Ret
                     {
                         return (*static_cast<std::remove_reference_t<Callable>*>(callable1))(
                             std::forward<Params>(params)...);
                     }),
          m_callable(&callable)
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
