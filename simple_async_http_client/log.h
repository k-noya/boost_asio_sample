// Copyright (c) 2021, k-noya
// Distributed under the BSD 3-Clause License.
// See accompanying file LICENSE
#ifndef SIMPLE_ASYNC_HTTP_CLIENT_LOG_H_
#define SIMPLE_ASYNC_HTTP_CLIENT_LOG_H_

#include <iostream>
#include <string>
#include <thread>

template <typename T>
void log(const T& message) {
  const auto thread_id = std::this_thread::get_id();
  std::cout << "[thread id: " << thread_id << "] ";
  std::cout << message << '\n';
}

#endif  // SIMPLE_ASYNC_HTTP_CLIENT_LOG_H_
