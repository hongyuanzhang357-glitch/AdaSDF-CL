#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <thread>
#include <vector>

namespace adasdf {

struct ParallelSamplingOptions {
  int threads = 1;
  std::size_t min_items_per_thread = 1;
};

struct ParallelSamplingStats {
  std::size_t item_count = 0;
  int threads_requested = 1;
  int threads_used = 1;
  bool parallel = false;
  double elapsed_ms = 0.0;
};

int normalizedThreadCount(int requested, std::size_t item_count);

template <typename Func>
ParallelSamplingStats parallelFor(
    std::size_t item_count,
    const ParallelSamplingOptions& options,
    Func&& func) {
  ParallelSamplingStats stats;
  stats.item_count = item_count;
  stats.threads_requested = options.threads;
  const auto t0 = std::chrono::steady_clock::now();
  int threads = normalizedThreadCount(options.threads, item_count);
  if (options.min_items_per_thread > 1 && item_count > 0) {
    const std::size_t max_threads_by_items =
        std::max<std::size_t>(1, item_count / options.min_items_per_thread);
    threads = std::min<int>(threads, static_cast<int>(max_threads_by_items));
  }
  stats.threads_used = threads;
  stats.parallel = threads > 1;

  if (item_count == 0) {
    stats.threads_used = 0;
  } else if (threads <= 1) {
    for (std::size_t i = 0; i < item_count; ++i) {
      func(i);
    }
  } else {
    std::atomic<std::size_t> first_failed_thread(
        static_cast<std::size_t>(threads));
    std::vector<std::exception_ptr> exceptions(static_cast<std::size_t>(threads));
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threads));
    const std::size_t base = item_count / static_cast<std::size_t>(threads);
    const std::size_t rem = item_count % static_cast<std::size_t>(threads);
    std::size_t begin = 0;
    for (int thread_index = 0; thread_index < threads; ++thread_index) {
      const std::size_t count =
          base + (static_cast<std::size_t>(thread_index) < rem ? 1 : 0);
      const std::size_t end = begin + count;
      workers.emplace_back([&, begin, end, thread_index]() {
        try {
          for (std::size_t i = begin; i < end; ++i) {
            if (first_failed_thread.load(std::memory_order_relaxed) !=
                static_cast<std::size_t>(threads)) {
              return;
            }
            func(i);
          }
        } catch (...) {
          exceptions[static_cast<std::size_t>(thread_index)] =
              std::current_exception();
          std::size_t expected = static_cast<std::size_t>(threads);
          first_failed_thread.compare_exchange_strong(
              expected,
              static_cast<std::size_t>(thread_index));
        }
      });
      begin = end;
    }
    for (std::thread& worker : workers) {
      worker.join();
    }
    const std::size_t failed = first_failed_thread.load();
    if (failed < exceptions.size() && exceptions[failed]) {
      std::rethrow_exception(exceptions[failed]);
    }
  }

  const auto t1 = std::chrono::steady_clock::now();
  stats.elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
  return stats;
}

}  // namespace adasdf
