// Copyright (c) 2021, k-noya
// Distributed under the BSD 3-Clause License.
// See accompanying file LICENSE
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#include "boost/system/error_code.hpp"
#include "simple_async_http_client/http.h"
#include "simple_async_http_client/http_client.h"
#include "simple_async_http_client/log.h"

int main() {
  bool is_completed{false};
  http_client::callback_type callback =
      [&is_completed](const boost::system::error_code& error,
                      const http_response& response) {
        if (error) {
          log(error);
        } else {
          log(response);
        }

        is_completed = true;
      };

  std::string hostname{"httpbin.org"};
  uint16_t port{80u};
  http_client client{hostname, port};

  header_block_t header_block{{"Accept", "application/json"}};
  client.async_get("/headers", header_block, callback);

  while (!is_completed) {
    log("wait for completion...");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
