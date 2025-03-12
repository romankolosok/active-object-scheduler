#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <chrono>
#include <random>
#include <memory>
#include <atomic>
#include <functional>
#include "active_object.h"
#include "future.h"

class Scheduler {
private:
    std::vector<std::shared_ptr<ActiveObject>> workers;
    int N_WORKERS;
    std::atomic<int> nextThread;

public:
    Scheduler(int n_workers)
        : N_WORKERS(n_workers), nextThread(0) {
        for (int i = 0; i < N_WORKERS; i++) {
            workers.push_back(std::make_shared<ActiveObject>());
        }
    }

    template<class F, class... Args>
    auto schedule(F&& f, Args&&... args) {
        using return_type = decltype(f(args...));
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        std::function<return_type()> func = [task]() -> return_type { return task(); };

        // circular buffer scheduling
        auto& worker = workers[nextThread++ % N_WORKERS];

        return worker->enqueue<return_type>(func);
    }
};

double dot_product(const std::vector<double>& v1, const std::vector<double>& v2, int
    start, int end)
{
    double local_dot_product = 0;
    for (int i = start; i < end; ++i)
    {
        local_dot_product += atan(sqrt(exp(v1[i]))) * atan(sqrt(exp(v2[i])));
    }
	return local_dot_product;
}

int main() {
    Scheduler scheduler(12);
    size_t N = 24;
    size_t size = 3000000;
    std::uniform_real_distribution<double> unif(-100, 100);
    std::default_random_engine re;
    std::vector<double> vec1(size);
    std::vector<double> vec2(size);
	std::queue<Future<double>> futures;

    for (int i = 0; i < size; ++i)
    {
        vec1[i] = unif(re);
        vec2[i] = unif(re);
    }

    std::vector<int> breakpoints(1, 0);
    size_t even_split = size / N;

    double dot_product_result = 0;

    for (auto s = even_split; s < size; s += even_split) {
        breakpoints.push_back(s);
    }
    breakpoints.push_back(size);

    auto start = clock();
    for (int i = 0; i < N; ++i)
    {
        if (i == N - 1)
        {
			futures.push(scheduler.schedule(dot_product, vec1, vec2, breakpoints[i], breakpoints[breakpoints.size() - 1]));
        }
        else
        {
			futures.push(scheduler.schedule(dot_product, vec1, vec2, breakpoints[i], breakpoints[i + 1]));
        }

    }

    while (!futures.empty()) {
        dot_product_result += futures.front().get();
        futures.pop();
    }

    auto end = clock();
    std::cout << "Dot product result: " << std::to_string(dot_product_result) << std::endl;
    std::cout << "Time taken: " << double(end - start) / CLOCKS_PER_SEC << " seconds" << std::endl;

    return 0;
}
