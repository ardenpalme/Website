#pragma once

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <queue>
#include <thread>
#include <functional>

using namespace std;

template<typename T>
class ThreadPool {
public:
    ThreadPool(int max_num_threads, function<void(T*)> job_hndl) : 
        max_num_threads{max_num_threads}, 
        job_hndl{job_hndl}
    {
        unique_lock lck{mtx};
        for(int i=0; i<max_num_threads; i++) {
            worker_threads.push_back(std::thread([this]() { this->thread_handler(); }));
        }
    }

    void enqueue(T *job) {
        {
            std::unique_lock<std::mutex> lck(mtx);
            job_queue.push(job);
        }
        cond_var.notify_one();
    }

private:
    int max_num_threads;
    std::condition_variable cond_var;
    mutex mtx;

    function<void(T*)> job_hndl;
    vector<std::thread> worker_threads;
    queue<T*> job_queue;

    void thread_handler() {
        while(1) {
            T *job;
            {
                std::unique_lock<std::mutex> lck(mtx);
                cond_var.wait(lck, [this]() {
                    return !job_queue.empty();
                });

                job = job_queue.front();
                job_queue.pop();
            }

            job_hndl(job);
            delete job;
        }
    }
};

