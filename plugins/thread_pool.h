#ifndef _PLUGIN_THREAD_POOL_H_
#define _PLUGIN_THREAD_POOL_H_

#include "lock_free_queue.h"
#include "plugin.h"
#include <atomic>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
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

    // 无锁队列容量，必须是 2 的幂。
    static constexpr std::size_t kQueueCapacity = 1 << 14;

    LockFreeQueue<std::function<void(void)>> m_tasks{kQueueCapacity};
    std::vector<std::thread> m_workers;
    std::atomic_bool m_stop{false};
    std::atomic_bool m_start{false};
    std::size_t m_size = 0;
    // 每次入队或停止都会自增，工作线程在队列为空时据此休眠与唤醒。
    alignas(64) std::atomic<std::size_t> m_wakeups{0};

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

        if (m_stop.load(std::memory_order_acquire))
            throw std::runtime_error("ThreadPool is stopped");

        std::function<void(void)> job = [task]() { (*task)(); };
        // 队列满时让出 CPU，等待工作线程腾出空位。
        while (!m_tasks.try_push(std::move(job)))
            std::this_thread::yield();

        m_wakeups.fetch_add(1, std::memory_order_release);
        m_wakeups.notify_one();
        return fut;
    }
};

}; // namespace plugins
#endif
