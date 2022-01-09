#include <boost/operators.hpp>
#include <functional>
#include <type_traits>
namespace {
template <typename...>
constexpr bool false_v = false;
}
namespace nyulan {
namespace utils {
template <typename tag, typename value_type, bool is_arithmetic>
struct PhantomBase_ {};

template <typename tag, typename value_type>
struct PhantomBase_<tag, value_type, true> : private boost::operators<PhantomBase_<tag, value_type, true>>,
                                             private boost::left_shiftable<PhantomBase_<tag, value_type, true>>,
                                             private boost::right_shiftable<PhantomBase_<tag, value_type, true>> {
    using ValueType = value_type;
    using Hash = std::hash<value_type>;
    using this_type = PhantomBase_<tag, value_type, true>;
    value_type value_;
    PhantomBase_(value_type value) : value_(value) {}
    PhantomBase_() {}
    template <typename T>
    explicit operator T() {
        if constexpr (std::is_convertible_v<value_type, T>) {
            return value_;
        } else {
            static_assert(false_v<T>);
        }
    }
    value_type value() { return value_; }

    bool operator<(const this_type& other) { return this->value_ < static_cast<value_type>(other); }
    bool operator==(const this_type& other) { return this->value_ == static_cast<value_type>(other); }
    this_type& operator+=(const this_type& other) {
        this->value_ += static_cast<value_type>(other);
        return *this;
    }
    this_type& operator-=(const this_type& other) {
        this->value_ -= static_cast<value_type>(other);
        return *this;
    }
    this_type& operator*=(const this_type& other) {
        this->value_ *= static_cast<value_type>(other);
        return *this;
    }
    this_type& operator/=(const this_type& other) {
        this->value_ /= static_cast<value_type>(other);
        return *this;
    }
    this_type& operator%=(const this_type& other) {
        this->value_ %= static_cast<value_type>(other);
        return *this;
    }
    this_type& operator|=(const this_type& other) {
        this->value_ |= static_cast<value_type>(other);
        return *this;
    }
    this_type& operator&=(const this_type& other) {
        this->value_ &= static_cast<value_type>(other);
        return *this;
    }
    this_type& operator^=(const this_type& other) {
        this->value_ ^= static_cast<value_type>(other);
        return *this;
    }
    this_type& operator<<=(const this_type& other) {
        this->value_ <<= static_cast<value_type>(other);
        return *this;
    }
    this_type& operator>>=(const this_type& other) {
        this->value_ >>= static_cast<value_type>(other);
        return *this;
    }
    this_type& operator++() {
        ++(this->value_);
        return *this;
    }
    this_type& operator--() {
        --(this->value_);
        return *this;
    }
    this_type& operator~() { return ~this->value_; }
};
template <typename tag, typename value_type>
struct PhantomBase_<tag, value_type, false> {
    using Hash = std::hash<value_type>;
    using ValueType = value_type;
    value_type value_;
    PhantomBase_(value_type value) : value_(value) {}
    PhantomBase_() {}
    operator value_type() { return value_; }
};

template <typename tag, typename value_type>
using Phantom = PhantomBase_<tag, value_type, std::is_arithmetic_v<value_type>>;

}  // namespace utils
}  // namespace nyulan