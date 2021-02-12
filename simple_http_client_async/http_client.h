// Copyright (c) 2021, k-noya
// Distributed under the BSD 3-Clause License.
// See accompanying file LICENSE
#ifndef SIMPLE_HTTP_CLIENT_ASYNC_HTTP_CLIENT_H_
#define SIMPLE_HTTP_CLIENT_ASYNC_HTTP_CLIENT_H_

#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <unordered_map>

#include "boost/asio.hpp"
#include "simple_http_client_async/http.h"

class http_client {
 public:
  http_client(const std::string& hostname, const uint16_t port);
  ~http_client();

  void get_async(const std::string& path, const header_block_t& header_block,
                 callback_t callback);

 private:
  uint64_t register_new_transaction(const callback_t& callback);
  void remove_transaction(uint64_t transaction_id);

  void on_send_request(const boost::system::error_code& error,
                       std::size_t bytes_transferred,
                       const uint64_t transaction_id);
  void on_receive_status_line(const boost::system::error_code& error,
                              std::size_t bytes_transferred,
                              const uint64_t transaction_id);
  void on_receive_response_header(const boost::system::error_code& error,
                                  std::size_t bytes_transferred,
                                  const uint64_t transaction_id);
  void on_receive_response_body(const boost::system::error_code& error,
                                std::size_t bytes_transferred,
                                const uint64_t transaction_id);

  boost::asio::io_context m_io_context;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
      m_executor_work_guard;

  std::string m_hostname;
  boost::asio::ip::tcp::socket m_socket;
  std::future<void> m_work_future;

  std::unordered_map<uint64_t, http_transaction_context_ptr> m_transaction_map;
  uint64_t m_next_id;
  std::mutex m_transaction_mutex;
};

#endif  // SIMPLE_HTTP_CLIENT_ASYNC_HTTP_CLIENT_H_
