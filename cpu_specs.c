#include "cpu_specs.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//// Helpers and global
#define KiB(x) ((x) * 1024)

__declspec(dllimport) void __stdcall __cpuidex(s32 out[4], s32 func, s32 subfunc);

struct cpu_specs    cpu_specs;
struct cpu_identity cpu_identity;


///////////////////////////////////////////////////////////////////////////////////////////////////
//// AMD functions
static void cpu_specs_amd_get_thread_specs(struct cpuid_ctx cpuid_ctx)
{
  s32 cpuid_out[4];

  if (cpuid_ctx.max_standard_func >= 0xB)
  {
    // Function 0xB provides useful data regarding the core and thread count, but seems to be
    // available since around the 1st quarter of 2019 with teh QuadCore AMD Ryzen 5 3500U. It
    // returns clearer, straightforward, unambiguous core and thread count, which is why it is
    // preferred. Earlier methods are more likely to be compatible with older CPUs, but depend
    // on hyperthreading/SMT CPUID flags, whose documentation is ambiguous.
    __cpuidex(cpuid_out, 0xB, 0x0);
    cpu_specs.threads_per_core = cpuid_out[EBX] & 0xFFFF;
    __cpuidex(cpuid_out, 0xB, 0x1);
    s32 total_thread_count = cpuid_out[EBX] & 0xFFFF;
    cpu_specs.core_count = total_thread_count / cpu_specs.threads_per_core;
  }
  else
  {
    __cpuidex(cpuid_out, 0x1, 0x0);

    // Although this is marked as "Hyper-threading technology" in AMD's manual, it is described as
    // as whether there is (1) or isn't (0) more than on thread per core OR more than one core per
    // compute unit (read "CPU"). So it is actually an indicator of multi-core CPUs for older CPUs.
    // See the DualCore AMD Athlon 64 X2 for instance, which has this bit set lacks hyperthreading
    // capabilities:
    // http://users.atw.hu/instlatx64/AuthenticAMD/AuthenticAMD0020FB1_K8_Manchester_CPUID.txt
    s32 hyperthreaded = (cpuid_out[EDX] >> 28) & 0b1;

    // If the branch was taken, there is no known way to determine the number of thread per core
    // programmatically. Since all AMD CPUs with simultaneous multithreading have 2 threads per
    // core as of december 2023, this should be alright . If this ever increases, 2 being the minimum value on hyperthreaded is
    // on
    cpu_specs.threads_per_core = hyperthreaded + 1;
    
    if (cpuid_ctx.max_extended_func >= 0x80000008)
    {
      // This is AMD's recommanded method to retrieve the total count of threads
      __cpuidex(cpuid_out, 0x80000008, 0);
      s32 total_thread_count = (cpuid_out[ECX] & 0xFF) + 1;
      cpu_specs.core_count = total_thread_count >> hyperthreaded;
    }
    else if (hyperthreaded)
    {
      s32 total_thread_count = (cpuid_out[EBX] >> 16) & 0xFF;
      s32 core_count         = total_thread_count >> 1;
      
      if (cpuid_ctx.max_extended_func >= 0x80000001)
      {
        __cpuidex(cpuid_out, 0x80000001, 0x0);
        if (cpuid_out[ECX] & (1 << 1))
        {
          cpu_specs.core_count = core_count;
        }
      }
      else
      {
        cpu_specs.core_count = core_count;
      }
    }
    // else if hyperthreaded is 0, there is a total of one thread
  }
}

static void cpu_specs_amd_get_cache_specs(struct cpuid_ctx cpuid_ctx)
{
  s32 cpuid_out[4];
  if (cpuid_ctx.max_extended_func >= 0x8000001D)
  {
    __cpuidex(cpuid_out, 0x80000001, 0x0);
    s32 topology_extensions_supported = cpuid_out[ECX] & (1 << 22);
    
    if (topology_extensions_supported)
    {
      // Compared to the 0x80000005-0x80000006 functions, the 0x8000001D function provides one
      // additional detail: the number of logical processors sharing a cache
      s32 subfunc = 0x0;
      __cpuidex(cpuid_out, 0x8000001D, subfunc);

      // Cache line sizes are provided for all enumerated caches, but in practice the cache line
      // size seems to always the same
      cpu_specs.cache_line_size = (cpuid_out[EBX] & 0x7F) + 1;
      
      do
      {
        // Filter out instruction caches, which cannot really be optimized at runtime. Only take
        // data or unified (data + instructions) caches into account, until all caches have been
        // enumerated
        if (cpuid_out[EAX] & 0x1)
        {
          s32 cache_level      = (cpuid_out[EAX] >> 5) & 0b11;
          s32 cache_set_ways   = (cpuid_out[EBX] >> 22) + 1;
          s32 cache_partitions = ((cpuid_out[EBX] >> 12) & 0x3FF) + 1;
          s32 cache_set_count  = cpuid_out[ECX] + 1;
          s32 cache_line_count = cache_partitions * cache_set_ways * cache_set_count;

          s32 cache_idx = cache_level - 1;
          struct cpu_cache_level_specs* cache_level_spec = &cpu_specs.cache_level_specs[cache_idx];
          
          cache_level_spec->data_cache_size     = cache_line_count * cpu_specs.cache_line_size;
          s32 attached_thread_count             = ((cpuid_out[EAX] >> 14) & 0xFFF) + 1;
          cache_level_spec->attached_core_count = attached_thread_count / cpu_specs.threads_per_core;
        }

        // Move to the next iteration, if the new subfunction is valid (cpuid_out[EAX] & 0xF != 0)
        subfunc++;
        __cpuidex(cpuid_out, 0x8000001D, subfunc);
      }
      while ((cpuid_out[EAX] & 0xF) != 0);
    }
  }
  else if (cpuid_ctx.max_extended_func >= 0x80000005)
  {
    // The count of threads attached to each cache is unknown. It is assumed that:
    // - there is 1 L1 cache per physical CPU core
    // - there is 1 L2 cache per physical CPU core
    // - there is 1 L3 cache for the whole CPU
    __cpuidex(cpuid_out, 0x80000005, 0x0);
    
    cpu_specs.cache_line_size = cpuid_out[ECX] & 0xFF;
    cpu_specs.cache_level_specs[L1].data_cache_size = ((cpuid_out[ECX] >> 24) & 0xFF) * KiB(1);
    cpu_specs.cache_level_specs[L1].attached_core_count = 1;
    if (cpuid_ctx.max_extended_func >= 0x80000006)
    {
      __cpuidex(cpuid_out, 0x80000006, 0x0);
      cpu_specs.cache_level_specs[L2].data_cache_size = ((cpuid_out[ECX] >> 16) & 0xFFFF) * KiB(1);
      cpu_specs.cache_level_specs[L2].attached_core_count = 1;

      // EDX[31:18] * 512KB <= l3_data_cache_size < (EDX[31:18] + 1) * 512KB
      cpu_specs.cache_level_specs[L3].data_cache_size = ((cpuid_out[EDX] >> 18) & 0x3FFFF) * KiB(512);
      cpu_specs.cache_level_specs[L3].attached_core_count = cpu_specs.core_count;
    }
  }
}

static void cpu_specs_amd_get_instructions(struct cpuid_ctx cpuid_ctx)
{
  s32 cpuid_out[4];
  
  // This is not officially documented by AMD, but appears in CPUID dumps of Zen4 CPUs, which
  // implement AVX512. So this is guesswork for now. See for instance:
  // http://users.atw.hu/instlatx64/AuthenticAMD/AuthenticAMD0A10F11_K19_Genoa_02_CPUID.txt
  if (cpuid_ctx.max_standard_func >= 0xB)
  {
    __cpuidex(cpuid_out, 0xD, 0x5);
    s32 avx512_available    = (cpuid_out[EAX] == 0x40) && (cpuid_out[EBX] == 0x340);
    cpu_specs.instructions |= avx512_available * AVX512F;
  }

  if (cpuid_ctx.max_extended_func >= 0xB)
  {
    __cpuidex(cpuid_out, 0x80000001, 0x0);
    cpu_specs.instructions |= ((cpuid_out[ECX] >> 21) & 0b1) * TBM;
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Intel functions
static void cpu_specs_intel_get_thread_specs(struct cpuid_ctx cpuid_ctx)
{
  s32 cpuid_out[4];
  if (cpuid_ctx.max_standard_func >= 0xB)
  {
    // If available, Intel recommands using function 0x1F, which is a superset of function 0xB
    s32 higher_func = (cpuid_ctx.max_standard_func >= 0x1F) ? 0x1F : 0xB;
    
    __cpuidex(cpuid_out, higher_func, 0x0);
    cpu_specs.threads_per_core = cpuid_out[EBX] & 0xFFFF;
    __cpuidex(cpuid_out, higher_func, 0x1);
    s32 total_thread_count = cpuid_out[EBX] & 0xFFFF;
    cpu_specs.core_count = total_thread_count / cpu_specs.threads_per_core;
  }
  else
  {
    __cpuidex(cpuid_out, 0x1, 0x0);
    s32 hyperthreaded = (cpuid_out[EDX] >> 28) & 0b1;
    cpu_specs.threads_per_core = hyperthreaded + 1;

    if (hyperthreaded)
    {
      s32 total_thread_count = ((cpuid_out[EBX] >> 16) & 0xFF);
      cpu_specs.core_count = total_thread_count / cpu_specs.threads_per_core;
    }
  }
}

static void cpu_specs_intel_get_cache_specs(struct cpuid_ctx cpuid_ctx)
{
  if (cpuid_ctx.max_standard_func >= 0x4)
  {
    s32 cpuid_out[4];
    s32 subfunc = 0x0;
    __cpuidex(cpuid_out, 0x4, subfunc);
    cpu_specs.cache_line_size = (cpuid_out[EBX] & 0x7F) + 1;
    
    while ((cpuid_out[EAX] & 0xF) != 0)
    {
      // Filter out instruction caches, which cannot really be optimized at runtime. Only take
      // data or unified (data + instructions) caches into account, until all caches have been
      // enumerated
      if (cpuid_out[EAX] & 0b1)
      {
        s32 cache_level      = (cpuid_out[EAX] >> 5) & 0b11;
        s32 cache_set_ways   = (cpuid_out[EBX] >> 22) + 1;
        s32 cache_partitions = ((cpuid_out[EBX] >> 12) & 0x3FF) + 1;
        s32 cache_set_count  = cpuid_out[ECX] + 1;
        s32 cache_line_count = cache_partitions * cache_set_ways * cache_set_count;

        s32 cache_idx = cache_level - 1;
        struct cpu_cache_level_specs* cache_level_spec = &cpu_specs.cache_level_specs[cache_idx];
        
        cache_level_spec->data_cache_size = cache_line_count * cpu_specs.cache_line_size;
        
        // There's no known way of retrieving the amount of active logical processors attached to a
        // cache at or below this function. Only the maximum amount of logical processors can be
        // retrieved. As a consequence, what follows may not always work in theory. For instance,
        // the maximum amount of logical processors attached to the L2 cache may be 8 out of 16
        // (4 cores out of 8), but may actually be 4 (2 cores). In this case,
        // cpu_specs.caches[L2].attached_core_count will be wrong.
        // TODO: investigate whether this can happen in practice for Intel CPUs
        s32 max_attached_thread_count = ((cpuid_out[EAX] >> 14) & 0xFFF) + 1;
        s32 max_attached_core_count   = max_attached_thread_count / cpu_specs.threads_per_core;
        if (max_attached_core_count <= cpu_specs.core_count)
        {
          cache_level_spec->attached_core_count = max_attached_core_count;
        }
        else
        {
          cache_level_spec->attached_core_count = cpu_specs.core_count;
        }
      }

      // Move to the next iteration, if the new subfunction is valid (cpuid_out[EAX] & 0xF != 0)
      subfunc++;
      __cpuidex(cpuid_out, 0x4, subfunc);
    }
  }
}

static void cpu_specs_intel_get_instructions(struct cpuid_ctx cpuid_ctx)
{
  if (cpuid_ctx.max_standard_func >= 0x7)
  {
    s32 cpuid_out[4];
    __cpuidex(cpuid_out, 0x7, 0x0);
    cpu_specs.instructions |= ((cpuid_out[EBX] & (1 << 16)) >> 16) * AVX512F;
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// CPU-independent functions
static void cpu_specs_get_common_instructions(struct cpuid_ctx cpuid_ctx)
{
  s32 cpuid_out[4];
  __cpuidex(cpuid_out, 0x1, 0x0);

  cpu_specs.instructions |= ((cpuid_out[EDX] >>  4) & 0b1) * RDTSCP;
  cpu_specs.instructions |= ((cpuid_out[EDX] >> 25) & 0b1) * SSE1;
  cpu_specs.instructions |= ((cpuid_out[EDX] >> 26) & 0b1) * SSE2;
  cpu_specs.instructions |= ((cpuid_out[ECX] >>  0) & 0b1) * SSE3;
  cpu_specs.instructions |= ((cpuid_out[ECX] >>  9) & 0b1) * SSSE3;
  cpu_specs.instructions |= ((cpuid_out[ECX] >> 12) & 0b1) * FMA3;
  cpu_specs.instructions |= ((cpuid_out[ECX] >> 19) & 0b1) * SSE4_1;
  cpu_specs.instructions |= ((cpuid_out[ECX] >> 20) & 0b1) * SSE4_2;
  cpu_specs.instructions |= ((cpuid_out[ECX] >> 23) & 0b1) * POPCNT;
  cpu_specs.instructions |= ((cpuid_out[ECX] >> 28) & 0b1) * AVX1;
  cpu_specs.instructions |= ((cpuid_out[ECX] >> 29) & 0b1) * F16C;

  if (cpuid_ctx.max_standard_func >= 0x7)
  {
    __cpuidex(cpuid_out, 0x7, 0x0);
    s32 bmi1_available = (cpuid_out[EBX] >> 3) & 0b1;
    cpu_specs.instructions |= bmi1_available * BMI1;
    cpu_specs.instructions |= bmi1_available * TZCNT;
    cpu_specs.instructions |= ((cpuid_out[EBX] >> 5) & 0b1) * AVX2;
    cpu_specs.instructions |= ((cpuid_out[EBX] >> 8) & 0b1) * BMI2;

    if (cpuid_ctx.max_extended_func >= 0x80000001)
    {
      __cpuidex(cpuid_out, 0x80000001, 0x0);
      cpu_specs.instructions |= ((cpuid_out[ECX] >> 5) & 0b1) * LZCNT;
    }
  }
}

static void cpu_specs_set_defaults()
{
  // If the following details of the CPU specs cannot be retrieved through CPUID, CPUID is either
  // unavailable, or its functions don't cover some of these features. The CPU may be old, so
  // assume low specs
  cpu_specs.cache_level_specs[L1].data_cache_size     = KiB(4);
  cpu_specs.cache_level_specs[L1].attached_core_count = 1;
  cpu_specs.cache_level_specs[L2].data_cache_size     = 0;
  cpu_specs.cache_level_specs[L2].attached_core_count = 0;
  cpu_specs.cache_level_specs[L3].data_cache_size     = 0;
  cpu_specs.cache_level_specs[L3].attached_core_count = 0;
  cpu_specs.cache_line_size                           = 64;
  cpu_specs.threads_per_core                          = 1;
  cpu_specs.core_count                                = 1;
  cpu_specs.instructions                              = 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// CPUID context
struct cpuid_ctx cpuid_ctx_get()
{
  struct cpuid_ctx cpuid_ctx;
  
  s32 cpuid_out[4];
  __cpuidex(cpuid_out, 0x0, 0x0);
  cpuid_ctx.max_standard_func = *(u32*)&cpuid_out[EAX];
  
  __cpuidex(cpuid_out, 0x80000000, 0x0);
  cpuid_ctx.max_extended_func = *(u32*)&cpuid_out[EAX];

  return cpuid_ctx;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// CPU specs
void cpu_specs_init()
{
  struct cpuid_ctx cpuid_ctx = cpuid_ctx_get();

  // Get details located at the same CPUID function regardless of the CPU's manucturer
  cpu_specs_set_defaults();
  cpu_specs_get_common_instructions(cpuid_ctx);

  s32 cpu_manufacturer_ecx;
  {
    s32 cpuid_out[4];
    __cpuidex(cpuid_out, 0x0, 0x0);
    cpu_manufacturer_ecx = cpuid_out[ECX];
  }

  if (cpu_manufacturer_ecx == CPU_MANUFACTURER_AMD)
  {
    // Call order matters here
    cpu_specs_amd_get_thread_specs(cpuid_ctx);
    cpu_specs_amd_get_cache_specs(cpuid_ctx);
    cpu_specs_amd_get_instructions(cpuid_ctx);
  }
  else if (cpu_manufacturer_ecx == CPU_MANUFACTURER_INTEL)
  {
    // Call order matters here
    cpu_specs_intel_get_thread_specs(cpuid_ctx);
    cpu_specs_intel_get_cache_specs(cpuid_ctx);
    cpu_specs_intel_get_instructions(cpuid_ctx);
  }
  // TODO: would it be interesting to check for lesser known manufacturers?
  // Check what their share is and was on the market
  // - Centaur/Zhaoxin
  // - Cyrix
  // - Geode
  // - Hygon
  // - Qualcomm
  // - Rise
  // - Vortex
  // - ...
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// CPU identity
void cpu_identity_set_defaults()
{
  cpu_identity.family          = 0;
  cpu_identity.model           = 0;
  cpu_identity.stepping        = 0;
  cpu_identity.manufacturer[0] = '\0';
  cpu_identity.name[11]        = '\0';
}

void cpu_identity_init()
{
  cpu_identity_set_defaults();
  
  s32 cpuid_out[4];
  __cpuidex(cpuid_out, 0x0, 0x0);

  // Copy the CPU's manufacturer string in groups of 4 characters.
  // EDX and ECX are intentionally swapped, this is how it is stored
  u32* manufacturer_4bytes = (u32*)cpu_identity.manufacturer;
  *manufacturer_4bytes++ = cpuid_out[EBX];
  *manufacturer_4bytes++ = cpuid_out[EDX];
  *manufacturer_4bytes   = cpuid_out[ECX];

  // Get the CPU family, model and stepping
  __cpuidex(cpuid_out, 0x1, 0x0);
  s32 family_model_stepping = cpuid_out[EAX];
  cpu_identity.family       = (family_model_stepping >> 8) & 0xF;
  cpu_identity.model        = (family_model_stepping >> 4) & 0xF;
  cpu_identity.stepping     = family_model_stepping & 0xF;

  if (cpu_identity.family == 0xF)
  {
    s32 extended_family = (family_model_stepping >> 20) & 0xFF;
    cpu_identity.family += extended_family;

    s32 extended_model = (family_model_stepping >> 12) & 0xF0;
    cpu_identity.model |= extended_model;
  }

  s32 manufacturer_ecx = *(s32*)(cpu_identity.manufacturer + 8);
  if ((manufacturer_ecx == CPU_MANUFACTURER_INTEL) && (cpu_identity.family == 0x6))
  {
    s32 extended_model = (family_model_stepping >> 12) & 0xF0;
    cpu_identity.model |= extended_model;
  }

  // If available, get the processor name string
  __cpuidex(cpuid_out, 0x80000000, 0x0);
  u32 cpuid_max_extended_func = *(u32*)&cpuid_out[EAX];
  if (cpuid_max_extended_func >= 0x80000004)
  {
    // Writes the 48 bytes (3 x 4 x 4 bytes) of the processor name 
    __cpuidex((s32*)(cpu_identity.name),      0x80000002, 0x0);
    __cpuidex((s32*)(cpu_identity.name + 16), 0x80000003, 0x0);
    __cpuidex((s32*)(cpu_identity.name + 32), 0x80000004, 0x0);
  }
}
