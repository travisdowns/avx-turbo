/*
 * cpuid.hpp
 */

#ifndef CPUID_HPP_
#define CPUID_HPP_

#include <cinttypes>
#include <string>

struct cpuid_result {
    std::uint32_t eax, ebx, ecx, edx;
    std::string to_string();
};

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


/** the highest supported leaf value */
uint32_t cpuid_highest_leaf();

cpuid_result cpuid(int leaf);

family_model get_family_model();

std::string get_brand_string();

#endif /* CPUID_HPP_ */
