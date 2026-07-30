/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 30/07/2026
 */

#pragma once

#include <cstdint>
#include <stdexcept>

///
/// Simple fixed point numerics library.
///
/// This is not intended to be a robust comprehensive implementation of fixed point arithmetic processing
/// rather a library specifically adapted to support fixed point processing for the Coral 66 language.
///
/// Usage example :-
///
/// int main() {
///
///    Explicit usage via the numeric::fixed namespace
///
///    numeric::fixed::DynamicFixed a(8.75, 13, 5);
///    numeric::fixed::DynamicFixed b(8.75, 13, 5);
///
///    numeric::fixed::DynamicFixed sum = a + b;
///    numeric::fixed::DynamicFixed target = sum.cast_to(16, 6);
///
///    std::cout << "Target Raw: " << target.raw_value << "\n"; // Outputs 1120
///  return 0
///

namespace numeric::fixed
{

class DynamicFixed
{
    // Helper function to force a 32-bit integer to behave like a signed 'bits' width integer
    static int32_t apply_bit_boundary(const int64_t val, const int bits)
    {
        if (bits >= 32)
            return static_cast<int32_t>(val);

        // 1. Mask out any garbage bits above our total_bits limit
        const uint32_t mask = (1ULL << bits) - 1;
        uint32_t masked_val = static_cast<uint32_t>(val) & mask;

        // 2. Perform manual sign-extension if the custom sign-bit is active
        if (const uint32_t sign_bit = 1U << (bits - 1); masked_val & sign_bit)
        {
            // Fill all upper bits with 1s for negative numbers
            masked_val |= ~mask;
        }
        return static_cast<int32_t>(masked_val);
    }

public:
    int32_t raw_value; // Underlying container is strictly 32-bit
    int total_bits;    // e.g., 13 bits
    int frac_bits;     // e.g., 5 bits

    // Constructor for real values
    DynamicFixed(const double val, const int total, const int frac) : total_bits(total), frac_bits(frac)
    {
        int64_t intermediate;
        if (frac_bits >= 0)
        {
            intermediate = static_cast<int64_t>(val * (1ULL << frac_bits));
        }
        else
        {
            intermediate = static_cast<int64_t>(val / (1ULL << -frac_bits));
        }
        raw_value = apply_bit_boundary(intermediate, total_bits);
    }

    // Constructor for raw integer values
    DynamicFixed(const int32_t raw, const int total, const int frac) : total_bits(total), frac_bits(frac)
    {
        raw_value = apply_bit_boundary(raw, total_bits);
    }

    // Rescale this variable to a new dynamic format
    DynamicFixed cast_to(const int new_total, const int new_frac) const
    {
        const int shift = new_frac - frac_bits;
        int64_t new_raw = raw_value;
        if (shift > 0)
            new_raw <<= shift;
        else if (shift < 0)
            new_raw >>= (-shift);
        return DynamicFixed(static_cast<int32_t>(new_raw), new_total, new_frac);
    }

    // Convert back to double for verification
    double to_double() const
    {
        if (frac_bits >= 0)
        {
            return static_cast<double>(raw_value) / (1ULL << frac_bits);
        }

        return static_cast<double>(raw_value) * (1ULL << -frac_bits);
    }

    // Addition (Result adopts LHS format)
    DynamicFixed operator+(const DynamicFixed& other) const
    {
        const DynamicFixed aligned_other = other.cast_to(this->total_bits, this->frac_bits);
        const int64_t res = static_cast<int64_t>(this->raw_value) + aligned_other.raw_value;
        return DynamicFixed(static_cast<int32_t>(res), total_bits, frac_bits);
    }

    // Subtraction (Result adopts LHS format)
    DynamicFixed operator-(const DynamicFixed& other) const
    {
        const DynamicFixed aligned_other = other.cast_to(this->total_bits, this->frac_bits);
        const int64_t res = static_cast<int64_t>(this->raw_value) - aligned_other.raw_value;
        return DynamicFixed(static_cast<int32_t>(res), total_bits, frac_bits);
    }

    // Multiplication (Result adopts LHS format)
    DynamicFixed operator*(const DynamicFixed& other) const
    {
        const int intermediate_frac = this->frac_bits + other.frac_bits;

        // Prevent 32-bit overflow mid-calculation
        int64_t temp_mult = static_cast<int64_t>(this->raw_value) * other.raw_value;

        if (const int shift_right = intermediate_frac - this->frac_bits; shift_right > 0)
            temp_mult >>= shift_right;
        else if (shift_right < 0)
            temp_mult <<= (-shift_right);

        return DynamicFixed(static_cast<int32_t>(temp_mult), this->total_bits, this->frac_bits);
    }

    // Division (Result adopts LHS format)
    DynamicFixed operator/(const DynamicFixed& other) const
    {
        if (other.raw_value == 0)
        {
            throw std::runtime_error("numerics::fixed::operator/ Division by zero in fixed-point operation.");
        }

        // Use 64-bit to safely shift the numerator left before dividing
        int64_t numerator = static_cast<int64_t>(this->raw_value);

        if (const int shift_left = other.frac_bits; shift_left > 0)
            numerator <<= shift_left;
        else if (shift_left < 0)
            numerator >>= (-shift_left);

        const int64_t temp_div = numerator / other.raw_value;

        return DynamicFixed(static_cast<int32_t>(temp_div), this->total_bits, this->frac_bits);
    }
};

} // namespace numeric::fixed
