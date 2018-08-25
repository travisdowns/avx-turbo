/*
 * avx-turbo.cpp
 */

#include "stats.hpp"
#include "tsc-support.hpp"
#include "table.hpp"
#include "cpu.h"
#include "msr-access.h"

#include "args.hxx"

#include <cstdlib>
#include <cinttypes>
#include <array>
#include <chrono>
#include <functional>
#include <cassert>
#include <thread>
#include <limits>
#include <vector>
#include <atomic>

#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>


#define MSR_IA32_MPERF 0x000000e7
#define MSR_IA32_APERF 0x000000e8

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
    x(ucomis,              "SSE scalar ucomis loop",       BASE)   \
    x(ucomis_vex,          "VEX scalar ucomis loop",       AVX2)   \
    x(scalar_iadd,         "Scalar integer adds",       BASE)   \
    x(avx128_mov_sparse,   "128-bit reg-reg mov",       AVX2)   \
    x(avx128_merge_sparse, "128-bit reg-reg merge mov", AVX512) \
    x(avx128_iadd,         "128-bit integer serial adds",      AVX2)   \
    x(avx128_iadd_t,       "128-bit integer parallel adds",    AVX2)   \
    x(avx128_imul,         "128-bit integer muls",      AVX2)   \
    x(avx128_fma_sparse ,  "128-bit 64-bit sparse FMAs",AVX2)   \
    x(avx128_fma ,         "128-bit serial DP FMAs" ,   AVX2)   \
    x(avx128_fma_t ,       "128-bit parallel DP FMAs" , AVX2)   \
    x(avx256_mov_sparse,   "256-bit reg-reg mov",       AVX2)   \
    x(avx256_merge_sparse, "256-bit reg-reg merge mov", AVX512) \
    x(avx256_iadd,         "256-bit integer serial adds",    AVX2)   \
    x(avx256_iadd_t,       "256-bit integer parallel adds",  AVX2)   \
    x(avx256_imul,         "256-bit integer muls",      AVX2)   \
    x(avx256_fma_sparse ,  "256-bit 64-bit sparse FMAs",AVX2)   \
    x(avx256_fma ,         "256-bit serial DP FMAs" ,   AVX2)   \
    x(avx256_fma_t,        "256-bit parallel DP FMAs" , AVX2)   \
    x(avx512_iadd,         "512-bit integer adds",    AVX512)   \
    x(avx512_imul,         "512-bit integer muls",    AVX512)   \
    x(avx512_fma_sparse ,  "512-bit 64-bit sparse FMAs", AVX512)   \
    x(avx512_fma ,         "512-bit serial DP FMAs" , AVX512)   \
    x(avx512_fma_t,        "512-bit parallel DP FMAs",AVX512)   \
    x(avx512_mov_sparse,   "512-bit reg-reg mov",     AVX512)       \
    x(avx512_merge_sparse, "512-bit reg-reg merge mov", AVX512)   \
    x(ucomis,              "SSE scalar ucomis loop",       BASE)   \
    x(ucomis_vex,          "VEX scalar ucomis loop",       AVX2)   \


#define DECLARE(f,...) cal_f f;

extern "C" {
// functions declared in asm-methods.asm
FUNCS_X(DECLARE);


// misc helpers
void zeroupper();

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
args::Flag arg_no_pin{parser, "--no-pin",
    "Don't try to pin threads to CPU - gives worse results but works around affinity issues on TravisCI", {"no-pin"}};
args::ValueFlag<std::string> arg_focus{parser, "TEST-ID", "Run only the specified test (by ID)", {"test"}};
args::ValueFlag<size_t> arg_iters{parser, "ITERS", "Run the test loop ITERS times (default 100000)", {"iters"}, 100000};
args::ValueFlag<size_t> arg_min_threads{parser, "MIN", "The minimum number of threads to use", {"min-threads"}, 1};
args::ValueFlag<size_t> arg_max_threads{parser, "MAX", "The maximum number of threads to use", {"max-threads"}};


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
        static double tsc_to_nanos = 1000000000.0 / tsc_freq();
        return diff * tsc_to_nanos;
    }

    static uint64_t tsc_freq() {
        static uint64_t freq = get_tsc_freq(arg_force_tsc_cal);
        return freq;
    }

};

/**
 * We pass an outer_clock to run_test which times outside the iteration of the innermost loop (i.e.,
 * it times around the loop that runs TRIES times), start should reset the state unless you want to
 * time warmup iterations.
 */
struct outer_timer {
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual ~outer_timer() {}
};

struct dummy_outer : outer_timer {
    static dummy_outer dummy;
    virtual void start() override {};
    virtual void stop() override {};
};
dummy_outer dummy_outer::dummy{};

/** lets you determine the actual frequency over any interval using the free-running APERF and MPERF counters */
struct aperf_ghz : outer_timer {
    uint64_t mperf_value, aperf_value, tsc_value;
    enum {
        STARTED, STOPPED
    } state;

    aperf_ghz() : mperf_value(0), aperf_value(0), tsc_value(0), state(STOPPED) {}

    uint64_t mperf() {
        return read(MSR_IA32_MPERF);
    }

    uint64_t aperf() {
        return read(MSR_IA32_APERF);
    }

    uint64_t read(uint32_t msr) {
        uint64_t value = -1;
        int res = read_msr_cur_cpu(msr, &value);
        assert(res == 0);
        return value;
    }

    virtual void start() override {
        assert(state == STOPPED);
        state = STARTED;
        mperf_value = mperf();
        aperf_value = aperf();
        tsc_value = rdtsc();
//        printf("started timer m: %lu\n", mperf_value);
//        printf("started timer a: %lu\n", aperf_value);
    };

    virtual void stop() override {
        assert(state == STARTED);
        mperf_value = mperf() - mperf_value;
        aperf_value = aperf() - aperf_value;
        tsc_value   = rdtsc() - tsc_value;
        state = STOPPED;
//        printf("stopped timer m: %lu (delta)\n", mperf_value);
//        printf("stopped timer a: %lu (delta)\n", aperf_value);
    };

    /** aperf / mperf ratio */
    double am_ratio() {
        assert(state == STOPPED);
        assert(mperf_value != 0 && aperf_value != 0);
//        printf("timer ratio m: %lu (delta)\n", mperf_value);
//        printf("timer ratio a: %lu (delta)\n", aperf_value);
        return (double)aperf_value / mperf_value;
    }

    /** mperf / tsc ratio, i.e., the % of the time the core was unhalted */
    double mt_ratio() {
        assert(state == STOPPED);
        assert(mperf_value != 0 && tsc_value != 0);
//        printf("timer ratio m: %lu (delta)\n", mperf_value);
//        printf("timer ratio a: %lu (delta)\n", aperf_value);
        return (double)mperf_value / tsc_value;
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
template <typename CLOCK, size_t TRIES = 101, size_t WARMUP = 3, size_t WARMDOWN = 10>
double run_test(cal_f* func, size_t iters, outer_timer& outer) {
    assert(iters % 100 == 0);

    std::array<typename CLOCK::delta_t, TRIES> results;

    for (size_t w = 0; w < WARMUP + 1; w++) {
        outer.start();
        for (size_t r = 0; r < TRIES; r++) {
            auto t0 = CLOCK::now();
            func(iters);
            auto t1 = CLOCK::now();
            func(iters * 2);
            auto t2 = CLOCK::now();
            results[r] = (t2 - t1) - (t1 - t0);
        }
        outer.stop();
    }

    for (size_t d = 0; d < WARMDOWN; d++) {
        func(iters);
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

std::vector<test_func> filter_tests(ISA isas_supported) {
    std::vector<test_func> ret;
    for (const auto& t : ALL_FUNCS) {
        if (should_run(t, isas_supported)) {
            ret.push_back(t);
        }
    }
    return ret;
}

struct result_holder {
    static constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    const test_func* test;
    bool valid;
    double    op_results;
    double    aperf_am;
    double    aperf_mt;

    result_holder(const test_func* test) : test(test), valid{false}, op_results(nan), aperf_am(nan), aperf_mt(nan) {}
};

struct hot_barrier {
    size_t break_count;
    std::atomic<size_t> current;
    hot_barrier(size_t count) : break_count(count) {}

    /* hot spin on the waiter count until it hits the break point */
    void wait() {
        current++;
        while (current.load() < break_count)
            ;
    }
};

struct test_thread {
    size_t id;
    hot_barrier* barrier;

    /* output */
    result_holder res;

    /* input */
    const test_func* test;
    size_t iters;
    bool use_aperf;


    test_thread(size_t id, hot_barrier& barrier, const test_func *test, size_t iters, bool use_aperf) :
        id{id}, barrier{&barrier}, res{test}, test{test}, iters{iters}, use_aperf{use_aperf} {}

    void operator()() {
//        printf("Running test in thread %d, this = %p\n", id, this);
        if (!arg_no_pin) {
            pin_to_cpu(id);
        }
        aperf_ghz aperf_timer;
        outer_timer& outer = use_aperf ? static_cast<outer_timer&>(aperf_timer) : dummy_outer::dummy;
        barrier->wait();
        res.op_results = run_test<RdtscClock>(test->func, iters, outer);
        res.aperf_am   = use_aperf ? aperf_timer.am_ratio() : 0.0;
        res.aperf_mt   = use_aperf ? aperf_timer.mt_ratio() : 0.0;
    }
};

template <typename E>
std::string result_string(const std::vector<result_holder>& results, const char* format, E e) {
    std::string s;
    for (const auto& result : results) {
        if (!s.empty()) s += ", ";
        s += table::string_format(format, e(result));
    }
    return s;
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

    bool is_root = (geteuid() == 0);
    bool use_aperf = is_root;
    printf("Running as root     : [%s]\n", is_root     ? "YES" : "NO ");
    printf("CPU pinning enabled : [%s]\n", !arg_no_pin ? "YES" : "NO ");

    ISA isas_supported = get_isas();
    printf("CPU supports AVX2   : [%s]\n", isas_supported & AVX2   ? "YES" : "NO ");
    printf("CPU supports AVX-512: [%s]\n", isas_supported & AVX512 ? "YES" : "NO ");
    printf("tsc_freq = %.1f MHz (%s)\n", RdtscClock::tsc_freq() / 1000000.0, get_tsc_cal_info(arg_force_tsc_cal));

    auto iters = arg_iters.Get();
    zeroupper();
    auto tests = filter_tests(isas_supported);

    size_t maxcpus = arg_max_threads ? arg_max_threads.Get() : get_nprocs();
    printf("Will test up to %lu CPUs\n", maxcpus);

    for (size_t thread_count = arg_min_threads.Get(); thread_count <= maxcpus; thread_count++) {

        std::vector<std::vector<result_holder>> results;

        // run
        for (size_t i = 0; i < tests.size(); i++) {
            const test_func& test = tests.at(i);
            hot_barrier barrier{thread_count}, dummy{0};
            std::vector<test_thread> test_threads(thread_count, {0, dummy, nullptr, 0, false});
            std::vector<std::thread> std_threads;
            for (size_t t = 0; t < thread_count; t++) {
                test_threads.at(t) = {t, barrier, &test, iters, use_aperf};
                std_threads.emplace_back(std::ref(test_threads.at(t)));
            }
//            printf("THIS OUT %p\n", &t);
            for (std::thread& t : std_threads) {
                t.join();
            }

            results.emplace_back();
            for (test_thread& t : test_threads) {
                results.back().push_back(t.res);
            }
            assert(results.back().size() == thread_count);
        }

        // report
        table::Table table;
        table.setColColumnSeparator(" | ");
        table.colInfo(2).justify = table::ColInfo::RIGHT;
        auto& r = table.newRow().add("ID").add("Description").add("Mops");
        if (use_aperf) {
            r.add("A/M-ratio");
            table.colInfo(3).justify = table::ColInfo::RIGHT;
            r.add("A/M-MHz");
            table.colInfo(4).justify = table::ColInfo::RIGHT;
            r.add("M/tsc-ratio");
            table.colInfo(5).justify = table::ColInfo::RIGHT;
        }

        assert(results.size() == tests.size());
        for (size_t i = 0; i < tests.size(); i++) {
            const auto& allres   = results.at(i);
            assert(allres.size() == thread_count);
            const auto& test = *allres.at(0).test;
            auto& r = table.newRow()
                    .add(test.id)
                    .add(test.description);

            r.add(result_string(allres, "%4.0f", [](const result_holder& r){ return r.op_results * 1000; }));
            if (use_aperf) {
                r.add(result_string(allres, "%5.2f", [](const result_holder& r){ return r.aperf_am; }));
                r.add(result_string(allres, "%.0f",  [](const result_holder& r){ return r.aperf_am / 1000000.0 * RdtscClock::tsc_freq(); }));
                r.add(result_string(allres, "%4.2f", [](const result_holder& r){ return r.aperf_mt; }));
            }
        }

        printf("============================== Threads: %2lu ==============================\n%s"
               "=========================================================================\n\n", thread_count, table.str().c_str());
    }



    return EXIT_SUCCESS;
}




