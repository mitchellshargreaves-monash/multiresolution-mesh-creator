#ifndef DVIDUTILS_UTILS_HPP
#define DVIDUTILS_UTILS_HPP

#include <cstdint>
#include "xtensor/xvectorize.hpp"

namespace dvidutils
{

    using std::int8_t;
    using std::int16_t;
    using std::int32_t;
    using std::int64_t;

    using std::uint8_t;
    using std::uint16_t;
    using std::uint32_t;
    using std::uint64_t;

    typedef float float32_t;
    typedef double float64_t;

    template <typename T> std::string dtype_short_name()  { return ""; }
    template <> std::string dtype_short_name<uint8_t>()   { return "u8"; }
    template <> std::string dtype_short_name<uint16_t>()  { return "u16"; }
    template <> std::string dtype_short_name<uint32_t>()  { return "u32"; }
    template <> std::string dtype_short_name<uint64_t>()  { return "u64"; }
    template <> std::string dtype_short_name<int8_t>()   { return "i8"; }
    template <> std::string dtype_short_name<int16_t>()  { return "iu16"; }
    template <> std::string dtype_short_name<int32_t>()  { return "i32"; }
    template <> std::string dtype_short_name<int64_t>()  { return "i64"; }

    template <> std::string dtype_short_name<float>()  { return "f32"; static_assert(sizeof(float) == 4, "Unknown architecture"); }
    template <> std::string dtype_short_name<double>()  { return "f64"; static_assert(sizeof(double) == 8, "Unknown architecture"); }

    template<typename T1, typename T2>
    std::string dtype_pair_name()
    {
        
        if (std::is_same<T1,T2>::value)
        {
            return dtype_short_name<T1>();
        }
        return dtype_short_name<T1>() + dtype_short_name<T2>();
    }

}
#endif // DVIDUTILS_UTILS_HPP
