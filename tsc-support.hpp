/*
 * tsc-support.cpp
 */

#include <cinttypes>
#include <string>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

static inline uint64_t rdtsc() {
    return __rdtsc();
}

struct cpuid_result {
    std::uint32_t eax, ebx, ecx, edx;
    std::string to_string();
};

cpuid_result cpuid(int leaf);


std::uint64_t get_tsc_freq();





