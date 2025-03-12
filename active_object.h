#ifndef ACTIVE_OBJECT_H
#define ACTIVE_OBJECT_H

#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "future.h"

class ActiveObject {
public:
    ActiveObject()
        : working(true), th(&ActiveObject::worker, this) {
    }

    ~ActiveObject() {
        stop();
        join();
    }

    template<typename R>
    Future<R> enqueue(std::function<R()> func) {
        auto promise = std::make_shared<Promise<R>>();
        auto task = std::make_shared<QueueEntry<R>>(func, promise);
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            work_queue.push(task);
        }
        cv.notify_one();
        return Future<R>(promise);
    }

    void stop() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            working = false;
        
            work_queue.push(nullptr);
        }
        cv.notify_one();
    }

    void join() {
        if (th.joinable()) {
            th.join();
        }
    }

private:
    class QEBase {
    public:
        virtual ~QEBase() {}
        virtual void run_and_complete() = 0;
    };

    template<typename R>
    class QueueEntry : public QEBase {
    public:
        using func_type = std::function<R()>;

        QueueEntry(func_type ft, std::shared_ptr<Promise<R>> prom)
            : task(std::move(ft)), promise(prom) {
        }

        void run_and_complete() override {
            promise->complete(task());
        }

    private:
        func_type task;
        std::shared_ptr<Promise<R>> promise;
    };

    void worker() {
        while (true) {
            std::shared_ptr<QEBase> entry;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                cv.wait(lock, [this] { return !work_queue.empty(); });
                entry = work_queue.front();
                work_queue.pop();
            }
            if (!entry) break; // Poison pill
            entry->run_and_complete();
        }
    }

    std::queue<std::shared_ptr<QEBase>> work_queue;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool working;
    std::thread th;
};

#endif
