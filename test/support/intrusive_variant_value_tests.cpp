/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/support/intrusive_variant_value.hpp"

#include "lib/support/abstract_intrusive_variant.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

namespace
{
class TrivBase : public coral66::AbstractIntrusiveVariant<TrivBase, class TrivDerived1, class TrivDerived2>
{
protected:
    template <class T>
    explicit TrivBase(std::in_place_type_t<T>) : AbstractIntrusiveVariant(std::in_place_type<T>)
    {
    }
};

class TrivDerived1 : public TrivBase
{
public:
    std::size_t size;
    float thing;

    TrivDerived1(const std::size_t size, const float thing) : TrivBase(std::in_place_type<TrivDerived1>), size(size), thing(thing)
    {
    }
};

class TrivDerived2 : public TrivBase
{
public:
    TrivDerived2() : TrivBase(std::in_place_type<TrivDerived2>) {}
};

} // namespace

namespace
{
class Base
    : public coral66::AbstractIntrusiveVariant<Base, class Derived1, class DestrSideEffect, class MoveConstrSideEffect>
{
protected:
    template <class T>
    explicit Base(std::in_place_type_t<T>) : AbstractIntrusiveVariant(std::in_place_type<T>)
    {
    }
};

class Derived1 : public Base
{
public:
    std::size_t size;
    float thing;

    // ReSharper disable once CppDFAConstantParameter
    Derived1(const std::size_t size, const float thing) : Base(std::in_place_type<Derived1>), size(size), thing(thing) {}
};

class DestrSideEffect : public Base
{
public:
    inline thread_local static int i = 0;

    DestrSideEffect() : Base(std::in_place_type<DestrSideEffect>)
    {
        i = 1;
    }

    ~DestrSideEffect()
    {
        i = -1;
    }
};

class MoveConstrSideEffect : public Base
{
public:
    inline thread_local static int i = 0;

    MoveConstrSideEffect() : Base(std::in_place_type<MoveConstrSideEffect>) {}

    MoveConstrSideEffect(MoveConstrSideEffect&& rhs) noexcept : Base(std::move(rhs))
    {
        i++;
    }

    MoveConstrSideEffect& operator=(MoveConstrSideEffect&&) noexcept = default;

    MoveConstrSideEffect(const MoveConstrSideEffect&) = delete;
    MoveConstrSideEffect& operator=(const MoveConstrSideEffect&) = delete;
};

} // namespace

TEST_CASE("IntrusiveVariantValue non trivial", "[IVV]")
{
    STATIC_REQUIRE(std::is_trivially_copyable_v<coral66::IntrusiveVariantValue<TrivBase>>);
    STATIC_REQUIRE(std::is_nothrow_copy_constructible_v<coral66::IntrusiveVariantValue<TrivBase>>);
    STATIC_REQUIRE(std::is_trivially_copy_assignable_v<coral66::IntrusiveVariantValue<TrivBase>>);
    STATIC_REQUIRE(std::is_nothrow_copy_assignable_v<coral66::IntrusiveVariantValue<TrivBase>>);
    STATIC_REQUIRE(std::is_trivially_move_constructible_v<coral66::IntrusiveVariantValue<TrivBase>>);
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<coral66::IntrusiveVariantValue<TrivBase>>);
    STATIC_REQUIRE(std::is_trivially_move_assignable_v<coral66::IntrusiveVariantValue<TrivBase>>);
    STATIC_REQUIRE(std::is_nothrow_move_assignable_v<coral66::IntrusiveVariantValue<TrivBase>>);
    STATIC_REQUIRE(std::is_trivially_destructible_v<coral66::IntrusiveVariantValue<TrivBase>>);
    SECTION("Simple usage")
    {
        coral66::IntrusiveVariantValue value = Derived1(5, 3.0);
        CHECK(value->is<Derived1>());
        CHECK(value->as<Derived1>().size == 5);
        CHECK(value->as<Derived1>().thing == 3.0);
    }
    SECTION("In place construction")
    {
        coral66::IntrusiveVariantValue value(std::in_place_type<Derived1>, 5, 3.0);
        CHECK(value->is<Derived1>());
        CHECK(value->as<Derived1>().size == 5);
        CHECK(value->as<Derived1>().thing == 3.0);
    }
    SECTION("Emplace construction")
    {
        coral66::IntrusiveVariantValue value(std::in_place_type<Derived1>, 5, 3.0);
        CHECK(value->is<Derived1>());
        CHECK(value->as<Derived1>().size == 5);
        CHECK(value->as<Derived1>().thing == 3.0);
        auto& derived = value.emplace<Derived1>(8, 5.0);
        CHECK(derived.size == 8);
        CHECK(derived.thing == 5.0);
    }
    SECTION("Move assignable once")
    {
        // ReSharper disable once CppEntityAssignedButNoRead
        coral66::IntrusiveVariantValue value = Derived1(5, 3.0);
        MoveConstrSideEffect::i = 0;
        // ReSharper disable once CppDFAUnusedValue
        value = MoveConstrSideEffect();
        CHECK(MoveConstrSideEffect::i == 1);
    }
    SECTION("Copyable from T")
    {
        auto d = Derived1(5, 3.0);
        coral66::IntrusiveVariantValue value = d;
        CHECK(value->is<Derived1>());
        value->as<Derived1>().size = 3;
        CHECK(value->as<Derived1>().size == 3);
        value->as<Derived1>().thing = 2;
        CHECK(value->as<Derived1>().thing == 2);
        value = d;
        CHECK(value->as<Derived1>().size == 5);
        CHECK(value->as<Derived1>().thing == 3.0);
    }
}
