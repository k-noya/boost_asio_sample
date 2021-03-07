// Copyright (c) 2021, k-noya
// Distributed under the BSD 3-Clause License.
// See accompanying file LICENSE
#ifndef SIMPLE_ASYNC_HTTP_CLIENT_LOG_H_
#define SIMPLE_ASYNC_HTTP_CLIENT_LOG_H_

#include <iostream>
#include <string>
#include <thread>

#include "boost/format.hpp"

template <typename T>
void log(T&& message) {
  const auto thread_id = std::this_thread::get_id();
  std::cout << boost::format("[thread id: %1%] %2%\n") % thread_id % message;
}

template <typename... Args>
void log(const std::string& format, Args&&... args) {
  const auto message = boost::format(format) % (... % args);
  log(message);
}

#endif  // SIMPLE_ASYNC_HTTP_CLIENT_LOG_H_
