/*
 * util.hpp
 */

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <utility>
#include <iterator>
#include <algorithm>
#include <cassert>

/*
 * Split a string delimited by sep.
 *
 * See https://stackoverflow.com/a/7408245/149138
 */
static inline std::vector<std::string> split(const std::string &text, const std::string &sep) {
  std::vector<std::string> tokens;
  std::size_t start = 0, end = 0;
  while ((end = text.find(sep, start)) != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = end + sep.length();
  }
  tokens.push_back(text.substr(start));
  return tokens;
}

/**
 * Like std::transform, but allocates and returns a std::vector for the result.
 */
template <typename Itr, typename F>
auto transformr(Itr begin, Itr end, F f) -> std::vector<decltype(f(std::declval<typename std::iterator_traits<Itr>::value_type>()))> {
    decltype(transformr(begin, end, f)) ret;
    ret.reserve(std::distance(begin, end));
    std::transform(begin, end, std::back_inserter(ret), f);
    return ret;
}

template <typename C, typename F>
auto transformv(const C& c, F f) -> std::vector<decltype(f(*c.begin()))> {
    return transformr(std::begin(c), std::end(c), f);
}

template <typename Itr>
static inline auto concurrency(Itr start, Itr stop) -> typename std::iterator_traits<Itr>::value_type {
    if (start == stop) {
        return {0, 0}; // early out for empty range simplifies some logic below
    }

    using T1 = decltype(concurrency(start, stop).first);

    struct event {
        T1 stamp;
        enum Type { START, STOP } type;
        event(T1 stamp, Type type) : stamp{stamp}, type{type} {}
    };

    std::vector<event> events;
    events.reserve(std::distance(start, stop));
    T1 sum_top{}, sum_bottom{};
    for (Itr i = start; i != stop; i++) {
        sum_top += i->second - i->first;
        events.emplace_back(i->first, event::START);
        events.emplace_back(i->second,event::STOP);
    }

    std::sort(events.begin(), events.end(), [](event l, event r){ return l.stamp < r.stamp; });

    size_t count = 0;
    const event* last_event = nullptr;
    for (auto& event : events) {
        assert(count > 0 || event.type == event::START);
        if (count != 0) {
            assert(last_event);
            T1 period = event.stamp - last_event->stamp;
            // active interval, accumulate the numerator and denominators
            sum_bottom += period;
        }
        last_event = &event;
        count += event.type == event::START ? 1 : -1;
    }

    assert(count == 0);

    return {sum_top, sum_bottom};
}

/**
 * Linearly remap value from the input range to the output range. That is, return the value that represents in outrange the
 * relative position of the input value in outrange.
 */
static inline double remap(double value, double inrange_start, double inrange_end, double outrange_start, double outrange_end) {
    return outrange_start + (outrange_end - outrange_start) / (inrange_end - inrange_start) * (value - inrange_start);
}

/**
 * The concurrency ratio for the pairs in the range [start, stop).
 *
 * Intuitively, a ratio of 1.0 means maximum overlap, while a ratio of 0.0 means all
 * the ranges were distinct.
 *
 * Simply a shortcut for (c.first - c.second) / (c.second * std::distance(start, stop)) where
 * c = concurrency(start, stop).
 */
template <typename Itr>
static inline double conc_ratio(Itr start, Itr stop) {
    size_t N = std::distance(start, stop);
    if (N == 1) {
        return 1.0; // special "by definition" case since remap doesn't work in this case
    }
    auto conc = concurrency(start, stop);
    // gives a ratio between N and 1 where N is the number of ranges
    double raw_ratio = conc.first/((double)conc.second);
    return remap(raw_ratio, 1.0, N, 0.0, 1.0);
}



#endif /* UTIL_HPP_ */
