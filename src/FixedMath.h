// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#ifndef _WIN32
#define _Inout_
#else
#include <sal.h> // defines _Inout_
#endif

class Fixed2_14
{
public:
    static const int8_t Precision = 14;
    static const int16_t PlusOne = 1 << Precision; 
    static const int16_t MinusOne = ~PlusOne + 1; 

public:    
    explicit Fixed2_14(int16_t value = 0) : value_(value) {}

    inline void SetRawValue(int16_t value)
    {
        value_ = value; 
    }

    inline int16_t GetRawValue() const
    {
        return value_;
    }

    int16_t ToInt() const
    {
        return value_; 
    }

private:
    int16_t value_;
};

class Fixed16_16
{
public:
    static const int8_t Precision = 16;
    static const int8_t Fixed2_14To16_16Shift = 2;
    static const int64_t RoundBit = 0x8000;
    static const int32_t NegativeInfinity = static_cast<int32_t>(0x80000000);
    static const int32_t PositiveInfinity = 0x7fffffff;
    static const int32_t PlusOne = 1 << Precision;
    static const int32_t MinusOne = (~PlusOne + 1);

private:
    static int32_t SaturateToInfinity(int64_t value)
    {
        return static_cast<int32_t>(std::min( std::max(value, static_cast<int64_t>(NegativeInfinity)), static_cast<int64_t>(PositiveInfinity) ));
    }   

    static inline bool ComputeInfinityShortCircuit(int32_t op1, int32_t op2, _Inout_ int32_t& result)
    {
        bool isOp1Infinite = (op1 == PositiveInfinity) || (op1 == NegativeInfinity);
        if (isOp1Infinite)
        {
            result = op1;
            return true;
        }

        bool isOp2Infinite = (op2 == PositiveInfinity) || (op2 == NegativeInfinity);
        if (isOp2Infinite)
        {
            result = op2;
            return true;
        }

        return false;
    }

public:
   
    explicit constexpr Fixed16_16(int32_t value = 0) : value_(value) {}

    Fixed16_16(Fixed16_16 const& other) = default;

    Fixed16_16& operator = (Fixed16_16 const& other) = default;

    static Fixed16_16 FromFixed2_14(Fixed2_14 value)
    {
        return Fixed16_16(static_cast<int32_t>(value.GetRawValue()) << Fixed2_14To16_16Shift);
    }

    static constexpr Fixed16_16 FromInt32(int32_t value)
    {
       return Fixed16_16(value << Precision);
    }

    static Fixed16_16 FromInt16(int16_t value)
    {
        return Fixed16_16(static_cast<int32_t>(value) << Precision);
    }

    static Fixed16_16 FromFloat(float value)
    {
        return Fixed16_16(SaturateToInfinity((static_cast<int64_t>(value * PlusOne * PlusOne) + RoundBit) >> Precision));
    }

    static int32_t FloatToFixedInt32(float value)
    {
        return Fixed16_16::FromFloat(value).GetRawValue();
    }

    static constexpr int32_t FixedInt32RoundToInt32(int32_t fixed) throw()
    {
        return (fixed + 0x8000) >> 16;
    }

    static constexpr int32_t Int32ToFixedInt32(int32_t value)
    {
        return value << Precision;
    }

    static float FixedInt32ToFloat(int32_t value)
    {
        return Fixed16_16(value).ToFloat();
    }

    inline constexpr int32_t GetRawValue() const
    {
        return value_;
    }

    inline void SetRawValue(int32_t value)
    {
        value_ = value; 
    }

    inline float ToFloat() const
    {
        static const float FixedToFloatMultiplier = 1.0f / Fixed16_16::PlusOne;
        if (value_ == PositiveInfinity)
        {
            return std::numeric_limits<float>::infinity();
        }
        else if (value_ == NegativeInfinity)
        {
            return -std::numeric_limits<float>::infinity();
        }
        return value_ * FixedToFloatMultiplier;
    }

    inline Fixed2_14 ToFixed2_14() const
    {
        // Round up to account for two lost bits of precision
        return Fixed2_14(static_cast<int16_t>((value_ + 2) >> Fixed2_14To16_16Shift));
    }

    inline int32_t ToRoundedInt32() const
    {
        return FixedInt32RoundToInt32(value_);
    }

    inline Fixed16_16 operator + (Fixed16_16 addend) const
    {
        int32_t result = 0;
        if (ComputeInfinityShortCircuit(value_, addend.value_, result))
        {
            return Fixed16_16(result);
        }
        int64_t sum = static_cast<int64_t>(value_) + static_cast<int64_t>(addend.value_);
        return Fixed16_16(SaturateToInfinity(sum));
    }

    inline Fixed16_16 operator - () const
    {
        if (value_ == PositiveInfinity)
        {
            return Fixed16_16(NegativeInfinity);
        }
        if (value_ == NegativeInfinity)
        {
            return Fixed16_16(PositiveInfinity);
        }

        return Fixed16_16(-value_);
    }

    inline Fixed16_16 operator - (Fixed16_16 subtrahend) const
    {
        int32_t result = 0;
        if (ComputeInfinityShortCircuit(value_, (-subtrahend).value_, result))
        {
            return Fixed16_16(result);
        }
        int64_t difference = static_cast<int64_t>(value_) - static_cast<int64_t>(subtrahend.value_);
        return Fixed16_16(SaturateToInfinity(difference));
    }

    inline Fixed16_16 operator * (Fixed16_16 multiplier) const
    {
        int64_t multiple = static_cast<int64_t>(value_) * static_cast<int64_t>(multiplier.value_) + RoundBit;
        int32_t product = SaturateToInfinity(multiple >> Precision);
        int32_t result;
        if (product != 0 && ComputeInfinityShortCircuit(value_, multiplier.value_, result))
        {
            return Fixed16_16(result);
        }
        return Fixed16_16(product);
    }    

private:
    int32_t value_;
};

inline bool operator < (Fixed16_16 lhs, Fixed16_16 rhs)
{
    return lhs.GetRawValue() < rhs.GetRawValue();
}

inline bool operator <= (Fixed16_16 lhs, Fixed16_16 rhs)
{
    return lhs.GetRawValue() <= rhs.GetRawValue();
}

inline bool operator > (Fixed16_16 lhs, Fixed16_16 rhs)
{
    return lhs.GetRawValue() > rhs.GetRawValue();
}

inline bool operator >= (Fixed16_16 lhs, Fixed16_16 rhs)
{
    return lhs.GetRawValue() >= rhs.GetRawValue();
}
