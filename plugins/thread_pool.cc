#include "thread_pool.h"

using namespace plugins;
void ThreadPool::work() {

    while (1) {
        std::unique_lock<std::mutex> ul(m_lock);
        if (m_tasks.empty()) {
            m_cv.wait(ul,
                      [this]() -> bool { return m_stop || !m_tasks.empty(); });
        }
        if (m_tasks.empty()) {
            if (m_stop) {
                return;
            }
            continue;
        }
        auto task = m_tasks.front();
        m_tasks.pop();
        ul.unlock();
        // 任务异常由 packaged_task 捕获并写入 future；此处兜底，
        // 避免任何意外异常逃出线程函数导致 std::terminate。
        try {
            task();
        } catch (...) {
        }
    }
}

void ThreadPool::initAndStart(size_t nums) {
    if (nums == 0)
        nums = 1;
    std::lock_guard<std::mutex> ul(m_lock);

    if (m_start)
        return;
    m_start = true;
    m_stop = false;
    m_size = nums;
    for (size_t i = 0; i < nums; ++i) {
        m_workers.emplace_back(&ThreadPool::work, this);
    }
}

void ThreadPool::stop() {

    {
        std::lock_guard<std::mutex> ul(m_lock);
        if (m_stop)
            return;
        m_stop = true;
    }
    m_cv.notify_all();

    for (auto &t : m_workers) {
        if (t.joinable())
            t.join();
    }
}
