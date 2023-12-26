#pragma once

#include "types.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//// enums and structs
enum cpuid_output_register
{
  EAX = 0,
  EBX = 1,
  ECX = 2,
  EDX = 3
};

enum cpu_manufacturer
{
  CPU_MANUFACTURER_AMD   = 0x444D4163, // "cAMD" of "AuthenticAMD"
  CPU_MANUFACTURER_INTEL = 0x6C65746E  // "ntel" of "GenuineIntel"
};

enum cache_level
{
  L1 = 0,
  L2 = 1,
  L3 = 2,
  CACHE_LEVEL_COUNT
};

enum cpu_instruction
{
  // SIMD extensions
  SSE1    = 1 <<  0,
  SSE2    = 1 <<  1,
  SSE3    = 1 <<  2, 
  SSSE3   = 1 <<  3,
  SSE4_1  = 1 <<  4,
  SSE4_2  = 1 <<  5,
  AVX1    = 1 <<  6,
  AVX2    = 1 <<  7,
  FMA3    = 1 <<  8,
  AVX512F = 1 <<  9,

  // Bitwise instruction
  POPCNT  = 1 << 10,
  LZCNT   = 1 << 11,
  TZCNT   = 1 << 12,
  BMI1    = 1 << 13,
  BMI2    = 1 << 14,
  TBM     = 1 << 15,

  // Utilities
  RDTSCP  = 1 << 16,
  F16C    = 1 << 17
};

struct cpuid_ctx
{
  // Highest valid standard CPUID function
  u32 max_standard_func;
  
  // Highest valid extended CPUID function
  u32 max_extended_func;
};

struct cpu_cache_level_specs
{
  // Size of the data cache
  s32 data_cache_size;

  // Count of logical thread sharing this cache
  s32 attached_core_count;
};

struct cpu_specs
{
  // Cache level specs accessed with enum cache_level's values. All physical caches at the same
  // level are expected to have the same features
  struct cpu_cache_level_specs cache_level_specs[CACHE_LEVEL_COUNT];

  // The cache line size seems to always be the same across all types of caches
  s32 cache_line_size;

  // Count of thread per CPU core
  s32 threads_per_core;

  // Count of logical processor in this CPU. This is always less than or equal to threads_per_core
  s32 core_count;

  // Instruction flags
  s32 instructions;
};

// Page Size Extension (PSE36) (simpler) | 4 MB pages, but no more 4 KB granularity after 4 GB
// PAE (Page Address Extension) | 2 MB pages

struct cpu_identity
{
  // CPU family, which represents one or more processors belonging to a group that possesses some
  // common definition for software or hardware purposes, as an 8-bit value
  s32 family;

  // CPU model, which is one instance of a processor family, as an 8-bit value
  s32 model;

  // CPU stepping, which is a particular version of a specific model, as a 4-bit value
  s32 stepping;

  // CPU manufacturer, identified with a fixed size, non-null-terminated string
  schar8 manufacturer[12];

  // CPU name, as a null-terminated string. This may remain empty for older CPUs
  schar8 name[48];
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Implementation
extern struct cpu_specs    cpu_specs;
extern struct cpu_identity cpu_identity;

// Initialize or refresh the global cpu_specs struct
void cpu_specs_init();

// Initialize or refresh the global cpu_identity struct.
// This is useful for statistics, dumps or to tune some performance-sensitive code to avoid certain
// instructions which may behave abnormally on specific CPUs
void cpu_identity_init();

// Initialize and return a cpuid_ctx structure.
// This is used by cpu_specs_init(), but may be useful for debugging purposes.
struct cpuid_ctx cpuid_ctx_get();
