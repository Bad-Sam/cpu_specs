#pragma once

// Should match reality 99% of the time. A notable exception is Win16 systems
// See https://en.cppreference.com/w/c/language/arithmetic_types#Integer_types
// TODO: change when BitIn(n) becomes well supported

// Integers
typedef signed   char      s8;
typedef signed   short     s16;
typedef signed   long      s32;
typedef signed   long long s64;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned long      u32;
typedef unsigned long long u64;

// Floating-point
typedef float  f32;
typedef double f64;

// Characters
typedef u8  uchar8;
typedef u16 uchar16;
typedef u32 uchar32;
typedef s8  schar8;
typedef s16 schar16;
typedef s32 schar32;

// Pointers
typedef s64 sptr;
typedef u64 uptr;

