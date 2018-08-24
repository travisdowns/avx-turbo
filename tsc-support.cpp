/*
 * tsc-support.cpp
 */

#include "tsc-support.hpp"

#include <cinttypes>
#include <string>
#include <cstdio>
#include <cassert>
#include <array>
#include <algorithm>

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

family_model gfm_inner() {
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

family_model get_family_model() {
    static family_model cached_family_model = gfm_inner();
    return cached_family_model;
}

uint64_t get_tsc_from_cpuid_inner() {
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

    return 0;
}

uint64_t get_tsc_from_cpuid() {
    static auto cached = get_tsc_from_cpuid_inner();
    return cached;
}


namespace Clock {
    static inline uint64_t nanos() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
    }
}

constexpr size_t SAMPLES = 101;
constexpr uint64_t DELAY_NANOS = 10000; // nanos 1us

uint64_t do_sample() {
    _mm_lfence();
    uint64_t  nsbefore = Clock::nanos();
    uint64_t tscbefore = rdtsc();
    while (nsbefore + DELAY_NANOS > Clock::nanos())
        ;
    uint64_t  nsafter = Clock::nanos();
    uint64_t tscafter = rdtsc();
    return (tscafter - tscbefore) * 1000000000u / (nsafter - nsbefore);
}

uint64_t tsc_from_cal() {
    std::array<uint64_t, SAMPLES * 2> samples;

    for (size_t s = 0; s < SAMPLES * 2; s++) {
        samples[s] = do_sample();
    }

    // throw out the first half of samples as a warmup
    std::array<uint64_t, SAMPLES> second_half;
    std::copy(samples.begin() + SAMPLES, samples.end(), second_half.begin());
    std::sort(second_half.begin(), second_half.end());

    // average the middle quintile
    auto third_quintile = second_half.begin() + 2 * SAMPLES/5;
    uint64_t sum = std::accumulate(third_quintile, third_quintile + SAMPLES/5, (uint64_t)0);

    return sum / (SAMPLES/5);
}

/**
 * TSC frequency detection is described in
 * Intel SDM Vol3 18.7.3: Determining the Processor Base Frequency
 *
 * Nominal TSC frequency = ( CPUID.15H.ECX[31:0] * CPUID.15H.EBX[31:0] ) ÷ CPUID.15H.EAX[31:0]
 */
uint64_t get_tsc_freq(bool force_calibrate) {
    uint64_t tsc;
    if (!force_calibrate && (tsc = get_tsc_from_cpuid())) {
        return tsc;
    }

    return tsc_from_cal();
}


const char* get_tsc_cal_info(bool force_calibrate) {
    if (!force_calibrate && get_tsc_from_cpuid()) {
        return "from cpuid leaf 0x15";
    } else {
        return "from calibration loop";
    }

}







