#ifndef _PLUGIN_THREAD_POOL_H_
#define _PLUGIN_THREAD_POOL_H_

#include "plugin.h"
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace plugins {

template <typename F>
concept VoidCallable = requires(F f) {
    { f() };
};

class ThreadPool : public Plugin<ThreadPool> {

    std::vector<std::thread> m_workers;
    std::queue<std::function<void(void)>> m_tasks;
    std::mutex m_lock;
    std::condition_variable m_cv;
    std::atomic_bool m_stop{false};
    std::atomic_bool m_start{false};
    std::size_t m_size;

    void work();

  public:
    enum class exceptions {
        NONE,
        POOL_STOPPED,
    };

    void initAndStart(size_t = std::thread::hardware_concurrency());

    void stop();

    ~ThreadPool() { stop(); }

    template <typename F>
    std::future<std::invoke_result_t<F>> submit(F &&f) {
        using T = std::invoke_result_t<F>;
        // 停止检查必须位于 packaged_task 内部，异常才会被存入 future，
        // 否则会逃出工作线程并触发 std::terminate。
        auto task = std::make_shared<std::packaged_task<T()>>(
            [this, fn = std::forward<F>(f)]() mutable -> T {
                if (m_stop)
                    throw exceptions::POOL_STOPPED;
                return std::invoke(fn);
            });

        std::future<T> fut = task->get_future();

        {
            std::lock_guard<std::mutex> l(m_lock);
            if (m_stop)
                throw std::runtime_error("ThreadPool is stopped");
            m_tasks.push([task]() { (*task)(); });
        }
        m_cv.notify_one();
        return fut;
    }
};

}; // namespace plugins
#endif // !_PLUGIN_THREAD_POOL_H_
