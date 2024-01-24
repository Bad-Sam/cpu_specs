# cpu_specs
Personal C library to identify common CPU features at runtime. This is useful to perform CPU dispatching.

```c
#include "cpu_specs.h"

int main()
{
  cpu_specs_init();

  s32 has_sse4_2   = cpu_specs.instructions & SSE4_2;
  s32 is_multicore = cpu_specs.core_count > 1;

  if (has_sse4_2 && is_multicore)
  {
    // Required CPU features available. Run the program
  }
  else
  {
    // Required CPU features unavailable. Present an error message explaining why the program
    // can't be run and exit early
  }
  
  return 0;
}
```

## Features
cpu_specs is designed to run on:
- Windows XP and above
- x86 architectures (32-bit and 64-bit)
- AMD or Intel CPUs (with defaults for unsupported CPU vendors)

cpu_specs can detect:
- L1, L2 and L3 data cache size and attached core count
- Cache line size
- Number of threads per physical core
- Total number of physical core in the CPU
- Available instructions:
  - SIMD instruction sets (SSE1, SSE2, SSE3, SSSE3, SSE4.1, SEE4.2, AVX1, AVX2, AVX512F, FMA3)
  - Bit manipulation instructions (POPCNT, LZNCT, TZCNT, BMI1, BMI2, TBM)
  - Half-precision conversion instructions (F16C)
  - Read time stamp counter instructions (RDTSC/RDTSCP)
- The CPU's name, manufacturer name, family, model and stepping


The detection of features is currently solely based on the
[CPUID instruction](https://en.wikipedia.org/wiki/CPUID), whose availability is checked at runtime.
When it is avaialble (see `cpuid_is_available()`):
- CPU features can be initialized with `cpu_specs_init()`, and are then accessible in the global
  variable `cpu_specs`.
- CPU identity fields can be initialized with `cpu_identity_init()`, and are then accessible in the
  global variable `cpu_identity`

When CPUID is unavailable, `cpu_specs` fields are initialized to represent a low-end CPU (see the top of
[`cpu_specs.c`](cpu_specs.c)).
  
No dynamic allocation is performed. The C standard library isn't used.  
 

## Possible improvements
- Properly detect and distinguish efficient and performance cores, such as with the
[Intel Core i7-12800HX](https://www.cpu-world.com/CPUs/Core_i7/Intel-Core%20i7%20i7-12800HX.html),
and return their accurate number of cores
- Add additional useful features to `cpu_specs` (page extensions?)
- Extend support to ARM architectures
- Extend support to UNIX-based operating systems (implies gcc support)

## Resources
- AMD: "AMD64 Architecture Programmer's Manual Volume 3: General-Purpose and System Instructions"
  (https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24594.pdf)
- Intel: "Intel 64 and IA-32 Architectures Software Developer's Manual Volume 2A: Instruction Set Reference"
  (https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html#nine-volume)
