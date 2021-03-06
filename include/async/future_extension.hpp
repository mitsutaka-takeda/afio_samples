// future_extension.hpp                       -*-C++-*-
#ifndef INCLUDED_FUTURE_EXTENSION
#define INCLUDED_FUTURE_EXTENSION

#include <future>

namespace async {
    namespace detail {
        // Adding then capability to std::future.
        template <typename T, typename Work>
        auto get_work_done(std::future<T> &f, Work &w)-> decltype(w(f.get())) {
            return w(f.get());
        }

        template <typename Work>
        auto get_work_done(std::future<void> &f, Work &w)-> decltype(w()) {
            f.wait();
            return w();
        }
    } // namespace detail

    template <typename T, typename Work>
    auto then(std::future<T> f, Work w) -> std::future<decltype(w(f.get()))> {
        return std::async([](std::future<T> f, Work w) {
                return detail::get_work_done(f,w);
            },
            std::move(f),
            std::move(w));
    }

} // namespace async

#endif /* INCLUDED_FUTURE_EXTENSION */
