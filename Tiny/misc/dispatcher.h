#pragma once
#include <concurrent_queue.h>   // PPL concurrent_queue
#include <functional>
#include <cstddef>
#include <mutex>
#include <unordered_set>
#include <typeindex>

#include "graphics/d3dcontext.h"

namespace tiny
{
	extern "C++" D3DContext gContext;

    class Dispatcher
    {
    public:
        Dispatcher() = default;
        ~Dispatcher() = default;

        template<typename F, typename... Args>
        void Post(F&& f, Args&&... args)
        {
            mTasks.push(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }

        void ProcessDispatchQueue() {
            std::function<void()> task;
            while (mTasks.try_pop(task))
            {
                task();
            }
        }

        void ProcessDispatchQueue(std::size_t maxTasks)
        {
            std::function<void()> task;
            while (maxTasks-- > 0 && mTasks.try_pop(task))
            {
                task();
            }
        }

    private:
        concurrency::concurrent_queue<std::function<void()>> mTasks;
    };
}