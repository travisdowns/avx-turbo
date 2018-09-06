/*
 * avx-turbo.cpp
 */

#include "stats.hpp"
#include "tsc-support.hpp"
#include "table.hpp"
#include "cpu.h"
#include "msr-access.h"

#include "args.hxx"
#include "util.hpp"

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
#include <deque>
#include <exception>

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
    x(ucomis,              "SSE scalar ucomis loop",       AVX512)   \
    x(ucomis_vex,          "VEX scalar ucomis loop",       AVX512)   \
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
    x(avx512_vpermw,       "512-bit serial  WORD permute",AVX512)   \
    x(avx512_vpermw_t,     "512-bit parallel WORD permute",AVX512)   \
    x(avx512_vpermd,       "512-bit serial DWORD permute",AVX512)   \
    x(avx512_vpermd_t,     "512-bit parallel DWORD permute",AVX512)   \
    x(avx512_mov_sparse,   "512-bit reg-reg mov",     AVX512)       \
    x(avx512_merge_sparse, "512-bit reg-reg merge mov", AVX512)   \


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
args::Flag arg_no_pin{parser, "no-pin",
    "Don't try to pin threads to CPU - gives worse results but works around affinity issues on TravisCI", {"no-pin"}};
args::Flag arg_verbose{parser, "verbose", "Output more info", {"verbose"}};
args::Flag arg_nobarrier{parser, "no-barrier", "Don't sync up threads before each test (debugging only)", {"no-barrier"}};
args::Flag arg_list{parser, "list", "List the available tests and their descriptions", {"list"}};
args::ValueFlag<std::string> arg_focus{parser, "TEST-ID", "Run only the specified test (by ID)", {"test"}};
args::ValueFlag<std::string> arg_spec{parser, "SPEC", "Run a specific type of test specified by a specification string", {"spec"}};
args::ValueFlag<size_t> arg_iters{parser, "ITERS", "Run the test loop ITERS times (default 100000)", {"iters"}, 100000};
args::ValueFlag<size_t> arg_min_threads{parser, "MIN", "The minimum number of threads to use", {"min-threads"}, 1};
args::ValueFlag<size_t> arg_max_threads{parser, "MAX", "The maximum number of threads to use", {"max-threads"}};
args::ValueFlag<uint64_t> arg_warm_ms{parser, "MILLISECONDS", "Warmup milliseconds for each thread after pinning (default 100)", {"warmup-ms"}, 100};


bool verbose;

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

    static uint64_t mperf() {
        return read(MSR_IA32_MPERF);
    }

    static uint64_t aperf() {
        return read(MSR_IA32_APERF);
    }

    static uint64_t read(uint32_t msr) {
        uint64_t value = -1;
        int res = read_msr_cur_cpu(msr, &value);
        assert(res == 0);
        return value;
    }

    /**
     * Return true iff APERF and MPERF MSR reads appear to work
     */
    static bool is_supported() {
        uint64_t dummy;
        return     read_msr_cur_cpu(MSR_IA32_MPERF, &dummy) == 0
                && read_msr_cur_cpu(MSR_IA32_APERF, &dummy) == 0;
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
 * The result of the run_test method, with only the stuff
 * that can be calculated from within that method.
 */
struct inner_result {
    /* calculated Mops value */
    double mops;
    uint64_t ostart_ts, oend_ts;
    uint64_t istart_ts, iend_ts; // start and end timestamps for the "critical" benchmark portion
};

/*
 * Calculate the frequency of the CPU based on timing a tight loop that we expect to
 * take one iteration per cycle.
 *
 * ITERS is the base number of iterations to use: the calibration routine is actually
 * run twice, once with ITERS iterations and once with 2*ITERS, and a delta is used to
 * remove measurement overhead.
 */
struct hot_barrier {
    size_t break_count;
    std::atomic<size_t> current;
    hot_barrier(size_t count) : break_count(count), current{0} {}

    /* increment the arrived count of the barrier (do this once per thread generally) */
    void increment() {
        current++;
    }

    /* return true if all the threads have arrived, never blocks */
    bool is_broken() {
        return current.load() == break_count;
    }

    /* increment and hot spin on the waiter count until it hits the break point, returns the spin count in case you care */
    long wait() {
        increment();
        long count = 0;
        while (!is_broken()) {
            count++;
        }
        return count;
    }
};

template <typename CLOCK, size_t TRIES = 101, size_t WARMUP = 3>
inner_result run_test(cal_f* func, size_t iters, outer_timer& outer, hot_barrier *barrier) {
    assert(iters % 100 == 0);

    std::array<typename CLOCK::delta_t, TRIES> results;

    inner_result result;

    result.ostart_ts = RdtscClock::now();
    for (size_t w = 0; w < WARMUP + 1; w++) {
        result.istart_ts = RdtscClock::now();
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
        result.iend_ts = RdtscClock::now();
    }

    for (barrier->increment(); !barrier->is_broken();) {
        func(iters);
    }
    result.oend_ts = RdtscClock::now();

    std::array<uint64_t, TRIES> nanos = {};
    std::transform(results.begin(), results.end(), nanos.begin(), CLOCK::to_nanos);
    DescriptiveStats stats = get_stats(nanos.begin(), nanos.end());

    result.mops = ((double)iters / stats.getMedian());
    return result;
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

/*
 * A test_spec contains the information needed to run one test. It is composed of
 * a list of test_funcs, which should be run in parallel on separate threads.
 */
struct test_spec {
    std::string name;
    std::string description;
    std::vector<test_func> thread_funcs;

    test_spec(std::string name, std::string description) : name{name}, description{description} {}

    /** how many threads/funcs in this test */
    size_t count() const { return thread_funcs.size(); }

    std::string to_string() const {
        std::string ret;
        for (auto& t : thread_funcs) {
            ret += t.id;
            ret += ',';
        }
        return ret;
    }
};

/**
 * If the user didn't specify any particular test spec, just create for every thread count
 * value T and runnable func, a spec with T copies of func.
 */
std::vector<test_spec> make_default_tests(ISA isas_supported) {
    std::vector<test_spec> ret;

    size_t maxcpus = arg_max_threads ? arg_max_threads.Get() : get_nprocs();
    printf("Will test up to %lu CPUs\n", maxcpus);

    for (size_t thread_count = arg_min_threads.Get(); thread_count <= maxcpus; thread_count++) {
        for (const auto& t : ALL_FUNCS) {
            if (should_run(t, isas_supported)) {
                test_spec spec(t.id, t.description);
                spec.thread_funcs.resize(thread_count, t); // fill with thread_count copies of t
                ret.push_back(std::move(spec));
            }
        }
    }

    return ret;
}

/* find the test that exactly matches the given ID or return nullptr if not found */
const test_func *find_one_test(const std::string id) {
    for (const auto& t : ALL_FUNCS) {
        if (id == t.id) {
            return &t;
        }
    }
    return nullptr;
}

std::vector<test_spec> make_from_spec(ISA) {
    std::string str = arg_spec.Get();
    if (verbose) printf("Making tests from spec string: %s\n", str.c_str());

    test_spec spec{str, "<multiple descriptions>"};
    for (auto& elem : split(str,",")) {
        if (verbose) printf("Elem: %s\n", elem.c_str());
        std::vector<std::string> halves = split(elem,"/");
        assert(halves.size() > 0);
        if (halves.size() > 2) {
            throw std::runtime_error(std::string("bad spec syntax in element: '" + elem + "'"));
        }
        int count = (halves.size() == 1 ? 1 : std::atoi(halves[1].c_str()));
        const test_func* test = find_one_test(halves[0]);
        if (!test) {
            throw std::runtime_error("couldn't find test: '" + halves[0] + "'");
        }

        spec.thread_funcs.insert(spec.thread_funcs.end(), count, *test);
    }

    return {spec};
}

std::vector<test_spec> filter_tests(ISA isas_supported) {
    if (!arg_spec) {
        return make_default_tests(isas_supported);
    } else {
        return make_from_spec(isas_supported);
    }
}

struct result {
    static constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    inner_result    inner;

    uint64_t  start_ts;  // start timestamp
    uint64_t    end_ts;  // end   timestamp

    /* optional stuff associated with outer_timer */
    double    aperf_am = nan;
    double    aperf_mt = nan;
};

struct result_holder {
    const test_spec* spec;
    std::vector<result> results; // will have spec.count() elements

    result_holder(const test_spec* spec) : spec(spec) {}

    /** calculate the overlap ratio based on the start/end timestamps */
    double get_overlap1() const {
        std::vector<std::pair<uint64_t, uint64_t>> ranges = transformv(results, [](const result& r){ return std::make_pair(r.start_ts, r.end_ts);} );
        return conc_ratio(ranges.begin(), ranges.end());
    }

    /** calculate the overlap ratio based on the start/end timestamps */
    double get_overlap2() const {
        std::vector<std::pair<uint64_t, uint64_t>> ranges = transformv(results, [](const result& r){ return std::make_pair(r.inner.istart_ts, r.inner.iend_ts);} );
        return conc_ratio(ranges.begin(), ranges.end());
    }

    /** calculate the inner overlap ratio based on the start/end timestamps */
    double get_overlap3() const {
        auto orange = transformv(results, [](const result& r){ return std::make_pair(r.inner.ostart_ts, r.inner.oend_ts);} );
        auto irange = transformv(results, [](const result& r){ return std::make_pair(r.inner.istart_ts, r.inner.iend_ts);} );
        return nconc_ratio(orange.begin(), orange.end(), irange.begin(), irange.end());
    }
};

struct warmup {
    uint64_t millis;
    warmup(uint64_t millis) : millis{millis} {}

    long warm() {
        int64_t start = (int64_t)RdtscClock::now();
        long iters = 0;
        while (RdtscClock::to_nanos(RdtscClock::now() - start) < 1000000u * millis) {
            iters++;
        }
        return iters;
    }
};

struct test_thread {
    size_t id;
    hot_barrier* start_barrier;
    hot_barrier* stop_barrier;

    /* output */
    result res;

    /* input */
    const test_func* test;
    size_t iters;
    bool use_aperf;

    std::thread thread;

    test_thread(size_t id, hot_barrier& start_barrier, hot_barrier& stop_barrier, const test_func *test, size_t iters, bool use_aperf) :
        id{id}, start_barrier{&start_barrier}, stop_barrier{&stop_barrier}, test{test},
        iters{iters}, use_aperf{use_aperf}, thread{std::ref(*this)}
    {
        // if (verbose) printf("Constructed test in thread %lu, this = %p\n", id, this);
    }

    test_thread(const test_thread&) = delete;
    test_thread(test_thread&&) = delete;
    void operator=(const test_thread&) = delete;

    void operator()() {
        // if (verbose) printf("Running test in thread %lu, this = %p\n", id, this);
        if (!arg_no_pin) {
            pin_to_cpu(id);
        }
        aperf_ghz aperf_timer;
        outer_timer& outer = use_aperf ? static_cast<outer_timer&>(aperf_timer) : dummy_outer::dummy;
        warmup w{arg_warm_ms.Get()};
        long warms = w.warm();
        if (verbose) printf("[%2lu] Warmup iters %lu\n", id, warms);
        if (!arg_nobarrier) {
            long count = start_barrier->wait();
            if (verbose) printf("[%2lu] Thread loop count: %ld\n", id, count);
        }
        res.start_ts = RdtscClock::now();
        res.inner = run_test<RdtscClock>(test->func, iters, outer, stop_barrier);
        res.end_ts = RdtscClock::now();
        res.aperf_am   = use_aperf ? aperf_timer.am_ratio() : 0.0;
        res.aperf_mt   = use_aperf ? aperf_timer.mt_ratio() : 0.0;
    }
};

template <typename E>
std::string result_string(const std::vector<result>& results, const char* format, E e) {
    std::string s;
    for (const auto& result : results) {
        if (!s.empty()) s += ", ";
        s += table::string_format(format, e(result));
    }
    return s;
}

void report_results(const std::vector<result_holder>& results_list, bool use_aperf) {
    // report
    table::Table table;
    table.setColColumnSeparator(" | ");
    table.colInfo(3).justify = table::ColInfo::RIGHT;
    table.colInfo(4).justify = table::ColInfo::RIGHT;
    auto& header = table.newRow().add("Cores").add("ID").add("Description")
            .add("OVRLP1").add("OVRLP2").add("OVRLP3").add("Mops");

    if (use_aperf) {
        size_t col = 6;
        header.add("A/M-ratio");
        table.colInfo(col + 0).justify = table::ColInfo::RIGHT;
        header.add("A/M-MHz");
        table.colInfo(col + 1).justify = table::ColInfo::RIGHT;
        header.add("M/tsc-ratio");
        table.colInfo(col + 2).justify = table::ColInfo::RIGHT;
    }

    for (const result_holder& holder : results_list) {
        auto spec = holder.spec;
        auto &row = table.newRow()
                                .add(spec->count())
                                .add(spec->name)
                                .add(spec->description)
                                .addf("%5.3f", holder.get_overlap1())
                                .addf("%5.3f", holder.get_overlap2())
                                .addf("%5.3f", holder.get_overlap3());

        auto& results = holder.results;
        row.add(result_string(results, "%4.0f", [](const result& r){ return r.inner.mops * 1000; }));
        if (use_aperf) {
            row.add(result_string(results, "%5.2f", [](const result& r){ return r.aperf_am; }));
            row.add(result_string(results, "%.0f",  [](const result& r){ return r.aperf_am / 1000000.0 * RdtscClock::tsc_freq(); }));
            row.add(result_string(results, "%4.2f", [](const result& r){ return r.aperf_mt; }));
        }
    }

    printf("%s\n", table.str().c_str());
}

void list_tests() {
    table::Table table;
    table.newRow().add("ID").add("Description");
    for (auto& t : ALL_FUNCS) {
        table.newRow().add(t.id).add(t.description);
    }
    printf("Available tests:\n\n%s\n", table.str().c_str());
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
    } catch (const args::ParseError& e) {
        printf("ERROR while parsing arguments: %s\n", e.what());
        printf("\nUsage:\n%s\n", parser.Help().c_str());
        exit(EXIT_FAILURE);
    }

    if (arg_list) {
        list_tests();
        exit(EXIT_SUCCESS);
    }

    verbose = arg_verbose;
    bool is_root = (geteuid() == 0);
    bool use_aperf = aperf_ghz::is_supported();
    printf("CPUID highest leaf  : [%2xh]\n", cpuid_highest_leaf());
    printf("Running as root     : [%s]\n", is_root     ? "YES" : "NO ");
    printf("MSR reads supported : [%s]\n", use_aperf   ? "YES" : "NO ");
    printf("CPU pinning enabled : [%s]\n", !arg_no_pin ? "YES" : "NO ");

    ISA isas_supported = get_isas();
    printf("CPU supports AVX2   : [%s]\n", isas_supported & AVX2   ? "YES" : "NO ");
    printf("CPU supports AVX-512: [%s]\n", isas_supported & AVX512 ? "YES" : "NO ");
    printf("tsc_freq = %.1f MHz (%s)\n", RdtscClock::tsc_freq() / 1000000.0, get_tsc_cal_info(arg_force_tsc_cal));

    auto iters = arg_iters.Get();
    zeroupper();
    auto specs = filter_tests(isas_supported);


    size_t last_thread_count = -1u;
    std::vector<result_holder> results_list;
    for (auto& spec : specs) {
        // if we changed the number of threads, spit out the accumulated output
        if (last_thread_count != -1u && last_thread_count != spec.count()) {
            // time to print results
            report_results(results_list, use_aperf);
            results_list.clear();
        }
        last_thread_count = spec.count();

        assert(!spec.thread_funcs.empty());
        if (verbose) printf("Running test spec: %s\n", spec.to_string().c_str());

        // run
        std::deque<test_thread> threads;
        hot_barrier start{spec.count()}, stop{spec.count()};
        for (auto& test : spec.thread_funcs) {
            threads.emplace_back(threads.size(), start, stop, &test, iters, use_aperf);
        }

        results_list.emplace_back(&spec);
        for (auto& t : threads) {
            t.thread.join();
            results_list.back().results.push_back(t.res);
        }
    }

    report_results(results_list, use_aperf);

    return EXIT_SUCCESS;
}




