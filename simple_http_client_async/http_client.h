// Copyright (c) 2021, k-noya
// Distributed under the BSD 3-Clause License.
// See accompanying file LICENSE
#ifndef SIMPLE_HTTP_CLIENT_ASYNC_HTTP_CLIENT_H_
#define SIMPLE_HTTP_CLIENT_ASYNC_HTTP_CLIENT_H_

#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>

#include "boost/asio.hpp"
#include "simple_http_client_async/http.h"

class http_client {
 public:
  http_client(const std::string& hostname, const uint16_t port);
  ~http_client();

  using callback_t = std::function<void(const boost::system::error_code&,
                                        const http_response)>;

  void async_get(const std::string& path, const header_block_t& header_block,
                 callback_t callback);

 private:
  boost::asio::io_context m_io_context;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
      m_executor_work_guard;
  std::future<void> m_future_event_loop;

  std::string m_hostname;
  std::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
};

#endif  // SIMPLE_HTTP_CLIENT_ASYNC_HTTP_CLIENT_H_
