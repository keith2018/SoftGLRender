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

  explicit ThreadPool(const size_t thread_cnt = std::thread::hardware_concurrency())
      : thread_cnt_(thread_cnt),
        threads_(new std::thread[thread_cnt_]) {
    CreateThreads();
  }

  ~ThreadPool() {
    WaitTasksFinish();
    running_ = false;
    JoinThreads();
  }

  inline size_t GetThreadCnt() const {
    return thread_cnt_;
  }

  template<typename F>
  void PushTask(const F &task) {
    tasks_cnt_++;
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      tasks_.push(std::function<void(size_t)>(task));
    }
  }

  template<typename F, typename... A>
  void PushTask(const F &task, const A &...args) {
    PushTask([task, args...] { task(args...); });
  }

  void WaitTasksFinish() const {
    while (true) {
      if (!paused) {
        if (tasks_cnt_ == 0)
          break;
      } else {
        if (TasksRunningCnt() == 0)
          break;
      }
      std::this_thread::yield();
    }
  }

  std::atomic<bool> paused{false};

 private:
  void CreateThreads() {
    for (size_t i = 0; i < thread_cnt_; i++) {
      threads_[i] = std::thread(&ThreadPool::TaskWorker, this, i);
    }
  }

  void JoinThreads() {
    for (size_t i = 0; i < thread_cnt_; i++) {
      threads_[i].join();
    }
  }

  size_t TasksQueuedCnt() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
  }

  size_t TasksRunningCnt() const {
    return tasks_cnt_ - TasksQueuedCnt();
  }

  bool PopTask(std::function<void(size_t)> &task) {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (tasks_.empty())
      return false;
    else {
      task = std::move(tasks_.front());
      tasks_.pop();
      return true;
    }
  }

  void TaskWorker(size_t thread_id) {
    while (running_) {
      std::function<void(size_t)> task;
      if (!paused && PopTask(task)) {
        task(thread_id);
        tasks_cnt_--;
      } else {
        std::this_thread::yield();
      }
    }
  }

 private:
  mutable std::mutex mutex_ = {};
  std::atomic<bool> running_{true};

  size_t thread_cnt_;
  std::unique_ptr<std::thread[]> threads_;

  std::queue<std::function<void(size_t)>> tasks_ = {};
  std::atomic<size_t> tasks_cnt_{0};
};

}
