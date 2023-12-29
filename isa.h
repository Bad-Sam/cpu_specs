#pragma once

#if defined(_MSC_VER)
#  if defined(_M_X64) || defined(_M_AMD64) || defined(_M_IA64)
#    define ISA_x64 1
#  elif defined(_M_IX86)
#    define ISA_x86 1
#  else
#    error Unsupported instruction set architecture
#  endif
#else
#  error Unsupported compiler
#endif
