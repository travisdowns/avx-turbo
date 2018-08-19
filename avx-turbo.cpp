/*
 * avx-turbo.cpp
 */

#include "stats.hpp"
#include "tsc-support.hpp"
#include "table.hpp"
#include "cpu.h"

#include "args.hxx"

#include <cstdlib>
#include <cinttypes>
#include <array>
#include <chrono>
#include <functional>
#include <cassert>

#include <error.h>

using std::uint64_t;
using namespace std::chrono;

using namespace Stats;

typedef void (cal_f)(uint64_t iters);

enum ISA {
    BASE   = 1,
    AVX2   = 2,
    AVX512 = 4
};

struct test_func {
    // function pointer to the test function
    cal_f* func;
    const char* id;
    const char* description;
    ISA isa;
};


#define FUNCS_X(x) \
    x(scalar_iadd, "Scalar integer adds",  BASE) \
    x(avx128_iadd, "128-bit integer adds", AVX2) \
    x(avx256_iadd, "256-bit integer adds", AVX2) \
    x(avx512_iadd, "512-bit integer adds", AVX512) \


#define DECLARE(f,...) cal_f f;

extern "C" {
// functions declared in asm-methods.asm
FUNCS_X(DECLARE)

}

#define MAKE_STRUCT(f, d, i) { f, #f, d, i },
const test_func ALL_FUNCS[] = {
FUNCS_X(MAKE_STRUCT)
};

void pin_to_cpu(int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) == -1) {
        error(EXIT_FAILURE, errno, "could not pin to CPU %d", cpu);
    }
}

/** args */
args::ArgumentParser parser{"avx-turbo: Determine AVX2 and AVX-512 downclocking behavior"};
args::HelpFlag help{parser, "help", "Display this help menu", {'h', "help"}};
args::Flag arg_force_tsc_cal{parser, "force-tsc-calibrate",
    "Force manual TSC calibration loop, even if cpuid TSC Hz is available", {"force-tsc-calibrate"}};
args::ValueFlag<std::string> arg_focus{parser, "TEST-ID", "Run only the specified test (by ID)", {"test"}};
args::ValueFlag<size_t> arg_iters{parser, "ITERS", "Run the test loop ITERS times (default 100000)", {"iters"}, 100000};


template <typename CHRONO_CLOCK>
struct StdClock {
    using now_t   = decltype(CHRONO_CLOCK::now());
    using delta_t = typename CHRONO_CLOCK::duration;

    static now_t now() {
        return CHRONO_CLOCK::now();
    }

    /* accept the result of subtraction of durations and convert to nanos */
    static uint64_t to_nanos(typename CHRONO_CLOCK::duration d) {
        return duration_cast<std::chrono::nanoseconds>(d).count();
    }
};

struct RdtscClock {
    using now_t   = uint64_t;
    using delta_t = uint64_t;

    static now_t now() {
        _mm_lfence();
        now_t ret = rdtsc();
        _mm_lfence();
        return ret;
    }

    /* accept the result of subtraction of durations and convert to nanos */
    static uint64_t to_nanos(now_t diff) {
        static double tsc_to_nanos = 1000000000.0 / get_tsc_freq(arg_force_tsc_cal);
        return diff * tsc_to_nanos;
    }

};

/*
 * Calculate the frequency of the CPU based on timing a tight loop that we expect to
 * take one iteration per cycle.
 *
 * ITERS is the base number of iterations to use: the calibration routine is actually
 * run twice, once with ITERS iterations and once with 2*ITERS, and a delta is used to
 * remove measurement overhead.
 */
template <typename CLOCK, size_t TRIES = 101, size_t WARMUP = 3>
double run_test(cal_f* func, size_t iters) {
    assert(iters % 100 == 0);

    std::array<typename CLOCK::delta_t, TRIES> results;

    for (size_t w = 0; w < WARMUP + 1; w++) {
        for (size_t r = 0; r < TRIES; r++) {
            auto t0 = CLOCK::now();
            func(iters);
            auto t1 = CLOCK::now();
            func(iters * 2);
            auto t2 = CLOCK::now();
            results[r] = (t2 - t1) - (t1 - t0);
        }
    }

    std::array<uint64_t, TRIES> nanos = {};
    std::transform(results.begin(), results.end(), nanos.begin(), CLOCK::to_nanos);
    DescriptiveStats stats = get_stats(nanos.begin(), nanos.end());

    double ghz = ((double)iters / stats.getMedian());
    return ghz;
}

ISA get_isas() {
    int ret = BASE;
    ret |= psnip_cpu_feature_check(PSNIP_CPU_FEATURE_X86_AVX2   ) ? AVX2   : 0;
    ret |= psnip_cpu_feature_check(PSNIP_CPU_FEATURE_X86_AVX512F) ? AVX512 : 0;
    return (ISA)ret;
}

bool should_run(const test_func& t, ISA isas_supported) {
    return (t.isa & isas_supported)
            && (!arg_focus || arg_focus.Get() == t.id);
}

int main(int argc, char** argv) {

    try {
        parser.ParseCLI(argc, argv);
        if (arg_iters.Get() % 100 != 0) {
            printf("ITERS must be a multiple of 100\n");
            exit(EXIT_FAILURE);
        }
    } catch (args::Help& help) {
        printf("%s\n", parser.Help().c_str());
        exit(EXIT_SUCCESS);
    }

    pin_to_cpu(0);
    ISA isas_supported = get_isas();
    printf("CPU supports AVX2   : [%s]\n", isas_supported & AVX2   ? "YES" : "NO ");
    printf("CPU supports AVX-512: [%s]\n", isas_supported & AVX512 ? "YES" : "NO ");
    printf("tsc_freq = %.1f MHz (%s)\n", 1000000000.0 / RdtscClock::to_nanos(1000000), get_tsc_cal_info(arg_force_tsc_cal));
    auto first = ALL_FUNCS[0].func;
    run_test<RdtscClock>(first, 1000000); // warmup


    table::Table table;
    table.colInfo(2).justify = table::ColInfo::RIGHT;
    table.newRow().add("ID").add("Description").add("MHz");
    for (const auto& test : ALL_FUNCS) {
        if (should_run(test, isas_supported)) {
            table.newRow()
                    .add(test.id)
                    .add(test.description)
                    .addf("%4.0f", run_test<RdtscClock>(test.func, arg_iters.Get()) * 1000);
        }
    }

    printf("==================\n%s================\n", table.str().c_str());

//
//    for (int i = 0; i < 2; i++) {
//        printf("hires:  GHz: %7.4f\n", CalcCpuFreq<StdClock<high_resolution_clock>>(first));
//        printf("steady: GHz: %7.4f\n", CalcCpuFreq<StdClock<steady_clock>         >(first));
//        printf("rdtsc : GHz: %7.4f\n", CalcCpuFreq<RdtscClock                     >(first));
//        printf("-----------------\n");
//    }



    return EXIT_SUCCESS;
}




