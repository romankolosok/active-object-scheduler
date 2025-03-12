#ifndef FUTURE_H
#define FUTURE_H

#include <mutex>
#include <condition_variable>
#include <memory>

template<typename R>
class Promise {
public:
    Promise()
        : value{}, completed{false}, mux{}
    { }

    bool is_complete() const {
        std::unique_lock<std::mutex> lck(mux);
        return completed;
    }

    R get() {
        std::unique_lock<std::mutex> lck(mux);
        while (! completed) {
            cv.wait(lck);
        }
        return value;
    }

    void complete(const R& v) {
        {
            std::unique_lock<std::mutex> lck(mux);
            value = v;
            completed = true;
        }
        cv.notify_all();
    }

private:
    R value;
    bool completed;
    mutable std::mutex mux;
    mutable std::condition_variable cv;

};

// R must be default-constructable and assignable.
template<typename R>
class Future {
public:
    Future()
        : promise {std::make_shared<Promise<R>>()}
    {}

	Future(std::shared_ptr<Promise<R>> p)
		: promise{ p }
	{
	}

    bool is_complete() const {
        return promise->is_complete();
    }

    R get() {
        return promise->get();
    }

    std::shared_ptr<Promise<R>> get_promise_ptr() {
        return promise;
    }

private:
    std::shared_ptr<Promise<R>> promise;
};

#endif
