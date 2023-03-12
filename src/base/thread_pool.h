/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace SoftGL {

class ThreadPool {
 public:

  explicit ThreadPool(const size_t threadCnt = std::thread::hardware_concurrency())
      : threadCnt_(threadCnt),
        threads_(new std::thread[threadCnt]) {
    createThreads();
  }

  ~ThreadPool() {
    waitTasksFinish();
    running_ = false;
    joinThreads();
  }

  inline size_t getThreadCnt() const {
    return threadCnt_;
  }

  template<typename F>
  void pushTask(const F &task) {
    tasksCnt_++;
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      tasks_.push(std::function<void(size_t)>(task));
    }
  }

  template<typename F, typename... A>
  void pushTask(const F &task, const A &...args) {
    pushTask([task, args...] { task(args...); });
  }

  void waitTasksFinish() const {
    while (true) {
      if (!paused) {
        if (tasksCnt_ == 0)
          break;
      } else {
        if (tasksRunningCnt() == 0)
          break;
      }
      std::this_thread::yield();
    }
  }

  std::atomic<bool> paused{false};

 private:
  void createThreads() {
    for (size_t i = 0; i < threadCnt_; i++) {
      threads_[i] = std::thread(&ThreadPool::taskWorker, this, i);
    }
  }

  void joinThreads() {
    for (size_t i = 0; i < threadCnt_; i++) {
      threads_[i].join();
    }
  }

  size_t tasksQueuedCnt() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
  }

  size_t tasksRunningCnt() const {
    return tasksCnt_ - tasksQueuedCnt();
  }

  bool popTask(std::function<void(size_t)> &task) {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (tasks_.empty())
      return false;
    else {
      task = std::move(tasks_.front());
      tasks_.pop();
      return true;
    }
  }

  void taskWorker(size_t threadId) {
    while (running_) {
      std::function<void(size_t)> task;
      if (!paused && popTask(task)) {
        task(threadId);
        tasksCnt_--;
      } else {
        std::this_thread::yield();
      }
    }
  }

 private:
  mutable std::mutex mutex_ = {};
  std::atomic<bool> running_{true};

  std::unique_ptr<std::thread[]> threads_;
  std::atomic<size_t> threadCnt_{0};

  std::queue<std::function<void(size_t)>> tasks_ = {};
  std::atomic<size_t> tasksCnt_{0};
};

}
