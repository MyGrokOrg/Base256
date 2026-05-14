#pragma once
#include <array>
#include <bit>
#include <cmath>
#include <compare>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace base256 {

class float256 {
public:
    using storage_type = std::array<std::uint64_t, 4>;

private:
    storage_type m_data{};
    static constexpr int EXP_BIAS = 262143;
    static constexpr int MAX_EXP = 524287;
    static constexpr std::uint64_t EXP_MASK = 0x7FFFF;

    bool get_sign() const noexcept { return (m_data[3] & (1ULL << 63)) != 0; }
    void set_sign(bool neg) noexcept { if (neg) m_data[3] |= (1ULL << 63); else m_data[3] &= ~(1ULL << 63); }
    int get_biased_exp() const noexcept { return (m_data[3] >> 44) & EXP_MASK; }
    void set_biased_exp(int e) noexcept { m_data[3] &= ~(EXP_MASK << 44); m_data[3] |= (static_cast<std::uint64_t>(e) & EXP_MASK) << 44; }

public:
    constexpr float256() noexcept = default;

    explicit float256(double v) noexcept {
        if (std::isnan(v)) { set_biased_exp(MAX_EXP); m_data[3] |= (1ULL << 40); return; }
        if (std::isinf(v)) { set_biased_exp(MAX_EXP); set_sign(std::signbit(v)); return; }
        if (v == 0.0) { set_sign(std::signbit(v)); return; }
        auto db = std::bit_cast<std::uint64_t>(v);
        bool s = (db >> 63) & 1;
        int de = (db >> 52) & 0x7FF;
        std::uint64_t dm = db & 0x000FFFFFFFFFFFFF;
        set_sign(s);
        if (de == 0 || de == 0x7FF) return;
        int ne = (de - 1023) + EXP_BIAS;
        if (ne <= 0) return;
        if (ne >= MAX_EXP) { set_biased_exp(MAX_EXP); return; }
        set_biased_exp(ne);
        std::uint64_t sig = (1ULL << 52) | dm;
        m_data[3] |= ((sig >> 25) & 0xFFFFFFFULL) << 16;
        m_data[2] = (sig << 39);
    }

    explicit float256(long double v) noexcept : float256(static_cast<double>(v)) {}
    explicit float256(std::string_view sv) : float256(std::stod(std::string(sv))) {}

    bool is_zero() const noexcept { return get_biased_exp() == 0 && m_data[0] == 0 && m_data[1] == 0 && m_data[2] == 0 && (m_data[3] & 0xFFFFFFF0ULL) == 0; }
    bool is_infinity() const noexcept { return get_biased_exp() == MAX_EXP && m_data[0] == 0 && m_data[1] == 0 && m_data[2] == 0 && (m_data[3] & 0xFFFFFFF0ULL) == 0; }
    bool is_nan() const noexcept { return get_biased_exp() == MAX_EXP && ((m_data[3] & 0xFFFFFFF0ULL) != 0 || m_data[0] || m_data[1] || m_data[2]); }
    bool is_negative() const noexcept { return get_sign(); }

    double to_double() const noexcept {
        if (is_nan()) return std::numeric_limits<double>::quiet_NaN();
        if (is_infinity()) return get_sign() ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
        if (is_zero()) return get_sign() ? -0.0 : 0.0;
        int ue = get_biased_exp() - EXP_BIAS;
        if (ue > 1023) return get_sign() ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
        if (ue < -1022) return get_sign() ? -0.0 : 0.0;
        std::uint64_t hm = (m_data[3] >> 16) & 0xFFFFFFFULL;
        std::uint64_t db = (get_sign() ? (1ULL << 63) : 0) | (static_cast<std::uint64_t>(ue + 1023) << 52) | ((hm << 25) & 0x000FFFFFFFFFFFFFULL);
        return std::bit_cast<double>(db);
    }

    std::string to_string(int prec = 17) const {
        if (is_nan()) return "nan";
        if (is_infinity()) return get_sign() ? "-inf" : "inf";
        if (is_zero()) return get_sign() ? "-0" : "0";
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.*g", prec, to_double());
        return std::string(buf);
    }

    std::strong_ordering operator<=>(const float256& o) const noexcept {
        if (is_nan() || o.is_nan()) return std::strong_ordering::less;
        if (get_sign() != o.get_sign()) return get_sign() ? std::strong_ordering::less : std::strong_ordering::greater;
        int e1 = get_biased_exp(), e2 = o.get_biased_exp();
        if (e1 != e2) return e1 < e2 ? std::strong_ordering::less : std::strong_ordering::greater;
        for (int i = 3; i >= 0; --i) if (m_data[i] != o.m_data[i]) return m_data[i] < o.m_data[i] ? std::strong_ordering::less : std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }

    bool operator==(const float256& o) const noexcept = default;

    float256 operator+(const float256& r) const noexcept {
        if (is_nan() || r.is_nan()) return float256(std::numeric_limits<double>::quiet_NaN());
        if (is_infinity() || r.is_infinity()) {
            if (is_infinity() && r.is_infinity() && get_sign() != r.get_sign()) return float256(std::numeric_limits<double>::quiet_NaN());
            return is_infinity() ? *this : r;
        }
        return float256(to_double() + r.to_double());
    }

    float256 operator-(const float256& r) const noexcept { return float256(to_double() - r.to_double()); }
    float256 operator*(const float256& r) const noexcept { return float256(to_double() * r.to_double()); }
    float256 operator/(const float256& r) const noexcept { return float256(to_double() / r.to_double()); }

    float256& operator+=(const float256& r) noexcept { *this = *this + r; return *this; }
    float256& operator-=(const float256& r) noexcept { *this = *this - r; return *this; }
    float256& operator*=(const float256& r) noexcept { *this = *this * r; return *this; }
    float256& operator/=(const float256& r) noexcept { *this = *this / r; return *this; }

    float256 operator-() const noexcept { float256 c = *this; c.set_sign(!get_sign()); return c; }

    storage_type raw_bits() const noexcept { return m_data; }
    void set_raw_bits(const storage_type& b) noexcept { m_data = b; }
};

namespace std {
template <>
class numeric_limits<base256::float256> {
public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
    static constexpr bool is_integer = false;
    static constexpr bool has_infinity = true;
    static constexpr bool has_quiet_NaN = true;
    static constexpr int digits = 237;
    static constexpr int digits10 = 71;
    static constexpr int radix = 2;
    static constexpr base256::float256 infinity() noexcept { base256::float256 f; f.set_biased_exp(524287); return f; }
    static constexpr base256::float256 quiet_NaN() noexcept { base256::float256 f; f.set_biased_exp(524287); f.m_data[3] |= (1ULL << 40); return f; }
};
}

namespace base256::literals {
inline float256 operator""_f256(const char* s, std::size_t n) { return float256(std::string_view(s, n)); }
}

} // namespace base256
