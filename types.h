#pragma once

// Should match reality 99% of the time. A notable exception is Win16 systems
// See https://en.cppreference.com/w/c/language/arithmetic_types#Integer_types
// TODO: change when BitIn(n) becomes well supported

typedef __int8           s8;
typedef __int16          s16;
typedef __int32          s32;
typedef __int64          s64;
typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;
typedef s8               schar8;

