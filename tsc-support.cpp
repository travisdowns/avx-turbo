/*
 * tsc-support.cpp
 */

#include "tsc-support.hpp"

#include <cinttypes>
#include <string>
#include <cstdio>
#include <cassert>

#include <error.h>

using std::uint32_t;

struct family_model {
    uint8_t family;
    uint8_t model;
    uint8_t stepping;
    std::string to_string() {
        std::string s;
        s += "family = " + std::to_string(family) + ", ";
        s += "model = " + std::to_string(model) + ", ";
        s += "stepping = " + std::to_string(stepping);
        return s;
    }
};

std::string cpuid_result::to_string() {
    std::string s;
    s += "eax = " + std::to_string(eax) + ", ";
    s += "ebx = " + std::to_string(ebx) + ", ";
    s += "ecx = " + std::to_string(ecx) + ", ";
    s += "edx = " + std::to_string(edx);
    return s;
}

cpuid_result cpuid(int leaf) {
    cpuid_result ret = {};
    asm ("cpuid"
            :
            "=a" (ret.eax),
            "=b" (ret.ebx),
            "=c" (ret.ecx),
            "=d" (ret.edx)
            :
            "a" (leaf),
            "c" (0)
    );
    return ret;
}

family_model get_family_model() {
    auto cpuid1 = cpuid(1);
    family_model ret;
    ret.family   = (cpuid1.eax >> 8) & 0xF;
    ret.model    = (cpuid1.eax >> 4) & 0xF;
    ret.stepping = (cpuid1.eax     ) & 0xF;
    if (ret.family == 15) {
        ret.family += (cpuid1.eax >> 20) & 0xFF;  // extended family
    }
    if (ret.family == 15 || ret.family == 6) {
        ret.model += ((cpuid1.eax >> 16) & 0xF) << 4; // extended model
    }
    return ret;
}

/**
 * TSC frequency detection is described in
 * Intel SDM Vol3 18.7.3: Determining the Processor Base Frequency
 *
 * Nominal TSC frequency = ( CPUID.15H.ECX[31:0] * CPUID.15H.EBX[31:0] ) ÷ CPUID.15H.EAX[31:0]
 */
uint64_t get_tsc_freq() {
    auto cpuid15 = cpuid(0x15);
    std::printf("cpuid = %s\n", cpuid15.to_string().c_str());

    if (cpuid15.ecx) {
        // the crystal frequency was present in ECX
        return cpuid15.ecx * cpuid15.ebx / cpuid15.eax;
    }

    // ecx == 0 means we have to use a hard-coded frequency based on the model and table provided by Intel
    // in 18.7.3
    auto family = get_family_model();
    std::printf("cpu: %s\n", family.to_string().c_str());

    assert(family.family == 6);

    if (family.model == 0x4E || family.model == 0x5E || family.model == 0x8E || family.model == 0x9E) {
        // skylake client or kabylake
        return (int64_t)24000000 * cpuid15.ebx / cpuid15.eax; // 24 MHz crystal clock
    }

    error(EXIT_FAILURE, 0, "unsupported cpu: family 6, model %d", family.model);
    __builtin_unreachable();
    return 0;
}







