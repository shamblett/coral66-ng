#ifndef DYNAMIC_FIXED_HPP
#define DYNAMIC_FIXED_HPP

#include <cstdint>
#include <stdexcept>

namespace numeric {

class DynamicFixed {
private:
    // Helper function to force a 32-bit integer to behave like a signed 'bits' width integer
    int32_t apply_bit_boundary(int64_t val, int bits) const {
        if (bits >= 32) return static_cast<int32_t>(val);

        // 1. Mask out any garbage bits above our total_bits limit
        uint32_t mask = (1ULL << bits) - 1;
        uint32_t masked_val = static_cast<uint32_t>(val) & mask;

        // 2. Perform manual sign-extension if the custom sign-bit is active
        uint32_t sign_bit = 1U << (bits - 1);
        if (masked_val & sign_bit) {
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
    DynamicFixed(double val, int total, int frac)
        : total_bits(total), frac_bits(frac) {
        int64_t intermediate;
        if (frac_bits >= 0) {
            intermediate = static_cast<int64_t>(val * (1ULL << frac_bits));
        } else {
            intermediate = static_cast<int64_t>(val / (1ULL << -frac_bits));
        }
        raw_value = apply_bit_boundary(intermediate, total_bits);
    }

    // Constructor for raw integer values
    DynamicFixed(int32_t raw, int total, int frac)
        : total_bits(total), frac_bits(frac) {
        raw_value = apply_bit_boundary(raw, total_bits);
    }

    // Rescale this variable to a new dynamic format
    DynamicFixed cast_to(int new_total, int new_frac) const {
        int shift = new_frac - frac_bits;
        int64_t new_raw = raw_value;
        if (shift > 0)       new_raw <<= shift;
        else if (shift < 0)  new_raw >>= (-shift);
        return DynamicFixed(static_cast<int32_t>(new_raw), new_total, new_frac);
    }

    // Convert back to double for verification
    double to_double() const {
        if (frac_bits >= 0) return static_cast<double>(raw_value) / (1ULL << frac_bits);
        else                return static_cast<double>(raw_value) * (1ULL << -frac_bits);
    }

    // Addition (Result adopts LHS format)
    DynamicFixed operator+(const DynamicFixed& other) const {
        DynamicFixed aligned_other = other.cast_to(this->total_bits, this->frac_bits);
        int64_t res = static_cast<int64_t>(this->raw_value) + aligned_other.raw_value;
        return DynamicFixed(static_cast<int32_t>(res), total_bits, frac_bits);
    }

    // Subtraction (Result adopts LHS format)
    DynamicFixed operator-(const DynamicFixed& other) const {
        DynamicFixed aligned_other = other.cast_to(this->total_bits, this->frac_bits);
        int64_t res = static_cast<int64_t>(this->raw_value) - aligned_other.raw_value;
        return DynamicFixed(static_cast<int32_t>(res), total_bits, frac_bits);
    }

    // Multiplication (Result adopts LHS format)
    DynamicFixed operator*(const DynamicFixed& other) const {
        int intermediate_frac = this->frac_bits + other.frac_bits;

        // Prevent 32-bit overflow mid-calculation
        int64_t temp_mult = static_cast<int64_t>(this->raw_value) * other.raw_value;

        int shift_right = intermediate_frac - this->frac_bits;
        if (shift_right > 0)       temp_mult >>= shift_right;
        else if (shift_right < 0)  temp_mult <<= (-shift_right);

        return DynamicFixed(static_cast<int32_t>(temp_mult), this->total_bits, this->frac_bits);
    }

    // Division (Result adopts LHS format)
    DynamicFixed operator/(const DynamicFixed& other) const {
        if (other.raw_value == 0) {
            throw std::runtime_error("Division by zero in fixed-point operation.");
        }

        // Use 64-bit to safely shift the numerator left before dividing
        int64_t numerator = static_cast<int64_t>(this->raw_value);
        int shift_left = other.frac_bits;

        if (shift_left > 0)       numerator <<= shift_left;
        else if (shift_left < 0)  numerator >>= (-shift_left);

        int64_t temp_div = numerator / other.raw_value;

        return DynamicFixed(static_cast<int32_t>(temp_div), this->total_bits, this->frac_bits);
    }
};

} // namespace numeric

#endif // DYNAMIC_FIXED_HPP

