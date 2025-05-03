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

        // Записать задачу в очередь
        template<typename F, typename... Args>
        void post(F&& f, Args&&... args) {
            tasks_.push(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }

        void ProcessDispatchQueue() {
            std::function<void()> task;
            while (tasks_.try_pop(task)) {
                task();
            }
        }

        void ProcessDispatchQueue(std::size_t maxTasks) {
            std::function<void()> task;
            while (maxTasks-- > 0 && tasks_.try_pop(task)) {
                task();
            }
        }

    private:
        concurrency::concurrent_queue<std::function<void()>> tasks_;
    };

    class FlushDispatcher {
    public:
		FlushDispatcher() = default;
        ~FlushDispatcher() = default;

        template<typename F, typename... Args>
        void Post(F&& f, Args&&... args) {
            tasks_.push(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
        }

        template<typename F, typename... Args>
        void PostOnce(F&& f, Args&&... args) {
            using FnType = std::decay_t<F>;
            std::type_index key = typeid(FnType);

            {
                std::lock_guard<std::mutex> lock(mtx_);
                if (!pending_.insert(key).second) {
                    // already pending, skip
                    return;
                }
            }

            // wrap it so that after execution we clear the pending flag
            auto bound = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            tasks_.push([this, key, bound]() mutable {
                bound();
                std::lock_guard<std::mutex> lock(mtx_);
                pending_.erase(key);
                });
        }

        void ProcessDispatchQueue() {
            if (!tasks_.empty()) {
                gContext.Flush();
                std::function<void()> task;
                while (tasks_.try_pop(task))
                    task();
            }
        }
    private:
        concurrency::concurrent_queue<std::function<void()>> tasks_;
        std::mutex                         mtx_;
        std::unordered_set<std::type_index> pending_;
    };
}