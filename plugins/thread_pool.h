#ifndef _PLUGIN_THREAD_POOL_H_
#define _PLUGIN_THREAD_POOL_H_

#include "plugin.h"
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
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

    template <typename T> std::future<T> submit(std::function<T()> f) {
        auto task = std::make_shared<std::packaged_task<T()>>(f);

        std::future<T> fut = task->get_future();

        {
            std::lock_guard<std::mutex> l(m_lock);
            if (m_stop)
                throw std::runtime_error("ThreadPool is stopped");
            m_tasks.push([task, this]() {
                if (m_stop) {
                    throw std::runtime_error("Thread stopped.");
                }
                (*task)();
            });
        }
        m_cv.notify_one();
        return fut;
    }
};

}; // namespace plugins
#endif // !_PLUGIN_THREAD_POOL_H_
