// Copyright (c) 2021, k-noya
// Distributed under the BSD 3-Clause License.
// See accompanying file LICENSE
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <string>

#include "boost/system/error_code.hpp"
#include "simple_async_http_client/http.h"
#include "simple_async_http_client/http_client.h"
#include "simple_async_http_client/log.h"

int main() {
  log("start main");

  bool is_completed{false};
  std::mutex mutex;
  std::condition_variable cond_complete{};

  http_client::callback_type callback =
      [&is_completed, &mutex, &cond_complete](
          const boost::system::error_code& error,
          const http_response& response) {
        if (error) {
          log(error);
        } else {
          log(response);
        }

        std::scoped_lock lock{mutex};
        is_completed = true;
        cond_complete.notify_one();
      };

  std::string hostname{"httpbin.org"};
  uint16_t port{80u};
  http_client client{hostname, port};

  header_block_t header_block{{"Accept", "application/json"}};
  client.async_get("/headers", header_block, callback);

  std::unique_lock lock{mutex};
  cond_complete.wait(lock, [&is_completed] { return is_completed; });

  log("finish main");

  return 0;
}
