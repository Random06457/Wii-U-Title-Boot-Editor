#pragma once

#include <functional>
#include <mutex>
#include <string>
#include "types.hpp"

class ProgressReport
{
public:
    void setCount(size_t count)
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_count = count;
    }
    void reportStep(size_t step)
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_step = step;
    }
    void reportStep(size_t step, std::string str)
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_step = step;
        m_msg = std::move(str);
    }
    void reportRatio(float ratio)
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_count = 100;
        m_step = (size_t)(ratio * 100);
    }
    void reportRatio(float ratio, std::string str)
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_count = 100;
        m_step = size_t(ratio * 100);
        m_msg = std::move(str);
    }
    void reportDone()
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_step = m_count;
    }

    float ratio()
    {
        std::lock_guard<std::mutex> lock(m_lock);
        return (float)m_step / (float)m_count;
    }
    bool isDone()
    {
        std::lock_guard<std::mutex> lock(m_lock);
        return m_step >= m_count;
    }
    bool isBusy() { return !isDone(); }
    std::string reportText()
    {
        std::lock_guard<std::mutex> lock(m_lock);
        return m_msg;
    }

private:
    size_t m_count;
    size_t m_step;
    std::string m_msg;
    std::mutex m_lock;
};
