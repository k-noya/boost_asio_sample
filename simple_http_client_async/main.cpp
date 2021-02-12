// Copyright (c) 2021, k-noya
// Distributed under the BSD 3-Clause License.
// See accompanying file LICENSE
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#include "boost/system/error_code.hpp"
#include "simple_http_client_async/http.h"
#include "simple_http_client_async/http_client.h"
#include "simple_http_client_async/log.h"

int main() {
  bool is_completed{false};
  callback_t callback = [&is_completed](const boost::system::error_code& error,
                                        const http_response response) {
    if (error) {
      std::cerr << error;
    } else {
      std::cout << response;
    }

    is_completed = true;
  };

  std::string hostname{"httpbin.org"};
  uint16_t port{80u};
  http_client client{hostname, port};
  header_block_t header_block{{"Accept", "application/json"}};
  client.get_async("/headers", header_block, callback);

  while (!is_completed) {
    log("wait for completion...");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
