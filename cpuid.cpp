/*
 * cpuid.cpp
 */

#include "cpuid.hpp"

#include <string.h>

using std::uint8_t;
using std::uint32_t;


std::string cpuid_result::to_string() {
    std::string s;
    s += "eax = " + std::to_string(eax) + ", ";
    s += "ebx = " + std::to_string(ebx) + ", ";
    s += "ecx = " + std::to_string(ecx) + ", ";
    s += "edx = " + std::to_string(edx);
    return s;
}

uint32_t cpuid_highest_leaf_inner() {
    return cpuid(0).eax;
}

uint32_t cpuid_highest_leaf() {
    static uint32_t cached = cpuid_highest_leaf_inner();
    return cached;
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

std::string get_brand_string() {
    auto check = cpuid(0x80000000);
    if (check.eax < 0x80000004) {
        return std::string("unkown (eax =") + std::to_string(check.eax) +")";
    }
    std::string ret;
    for (uint32_t eax : {0x80000002, 0x80000003, 0x80000004}) {
        char buf[17];
        auto fourchars = cpuid(eax);
        memcpy(buf +  0, &fourchars.eax, 4);
        memcpy(buf +  4, &fourchars.ebx, 4);
        memcpy(buf +  8, &fourchars.ecx, 4);
        memcpy(buf + 12, &fourchars.edx, 4);
        buf[16] = '\0';
        ret += buf;
    }
    return ret;
}

