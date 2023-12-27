# cpu_specs
Personal C library to identify common CPU features at runtime. This is useful to perform CPU dispatching.

```c
#include "cpu_specs.h"

int main()
{
  cpu_specs_init();

  s32 has_sse4_2      = cpu_specs.instructions & SSE4_2;
  s32 is_multicore    = cpu_specs.core_count > 1;
  s32 can_run_program = has_sse4_2 && is_multicore;

  if (can_run_program)
  {
    // Required CPU features available. Run the program
  }
  else
  {
    // Required CPU features unavailable. Present an error message explaining why the program can't
    // be run and exit early
  }
  
  return 0;
}
```

## Features
cpu_specs is designed to run on:
- Windows XP and above
- x86 architectures (32- and 64-bits)
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

CPU features are initialized with `cpu_specs_init()`, and are then accessible in the global
variable `cpu_specs`.  
CPU identity is initialized with `cpu_identity_init()`, and are then accessible in the global
variable `cpu_identity`.  
  
No dynamic allocation are performed. The C standard library isn't used.  
  
The detection of features is currently solely based on the [CPUID instruction](https://en.wikipedia.org/wiki/CPUID).  


## Possible improvements
- Check for CPUID instruction availability before using it
- Use Win32 functions for features that couldn't be fetched through CPUID, or when CPUID is unavailable
- Add additional useful features to `cpu_specs` (page extensions?)
- Extend support to ARM architectures
- Extend support to UNIX-based operating systems (implies gcc support)

## Resources
- AMD: "AMD64 Architecture Programmer's Manual Volume 3: General-Purpose and System Instructions"
  (https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24594.pdf)
- Intel: "Intel 64 and IA-32 Architectures Software Developer's Manual Volume 2A: Instruction Set Reference"
  (https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html#nine-volume)
