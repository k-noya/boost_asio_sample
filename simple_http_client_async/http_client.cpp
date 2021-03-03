// Copyright (c) 2021, k-noya
// Distributed under the BSD 3-Clause License.
// See accompanying file LICENSE
#include "simple_http_client_async/http_client.h"

#include <cstdint>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "boost/algorithm/string/classification.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/asio.hpp"
#include "simple_http_client_async/http.h"
#include "simple_http_client_async/log.h"

namespace {

using asio_socket = boost::asio::ip::tcp::socket;
using socket_ptr = std::shared_ptr<asio_socket>;

struct transaction_context {
  transaction_context(socket_ptr socket,
                      const http_client::callback_t& completion_callback)
      : m_socket{socket},
        m_response{},
        m_read_buffer{},
        m_completion_callback{completion_callback} {};
  socket_ptr m_socket;
  http_response m_response;
  std::string m_read_buffer;
  http_client::callback_t m_completion_callback;
};

using transaction_context_ptr = std::shared_ptr<transaction_context>;

socket_ptr construct_connected_socket(boost::asio::io_context* io_context,
                                      const std::string& hostname,
                                      const uint16_t port) {
  boost::asio::ip::tcp::resolver resolver{*io_context};
  const auto results = resolver.resolve(hostname, std::to_string(port));
  const auto remote_endpoint = results.begin()->endpoint();

  socket_ptr socket = std::make_shared<asio_socket>(*io_context);
  socket->connect(remote_endpoint);
  return socket;
}

// Callback handler
void on_send_request(const boost::system::error_code& error,
                     std::size_t bytes_transferred,
                     transaction_context_ptr txn_context);
void on_receive_status_line(const boost::system::error_code& error,
                            std::size_t bytes_transferred,
                            transaction_context_ptr txn_context);
void on_receive_response_header(const boost::system::error_code& error,
                                std::size_t bytes_transferred,
                                transaction_context_ptr txn_context);
void on_receive_response_body(const boost::system::error_code& error,
                              std::size_t bytes_transferred,
                              transaction_context_ptr txn_context);

void on_send_request(const boost::system::error_code& error,
                     std::size_t bytes_transferred,
                     transaction_context_ptr txn_context) {
  if (error) {
    txn_context->m_completion_callback(error, txn_context->m_response);
    return;
  }

  log("succeed to request");

  txn_context->m_read_buffer.clear();

  const auto handler = [txn_context](const boost::system::error_code& error,
                                     std::size_t bytes_transferred) {
    on_receive_status_line(error, bytes_transferred, txn_context);
  };
  const auto delimiter = NEW_LINE;
  boost::asio::async_read_until(
      *txn_context->m_socket,
      boost::asio::dynamic_buffer(txn_context->m_read_buffer), delimiter,
      handler);

  return;
}

void on_receive_status_line(const boost::system::error_code& error,
                            std::size_t bytes_transferred,
                            transaction_context_ptr txn_context) {
  if (error) {
    txn_context->m_completion_callback(error, txn_context->m_response);
    return;
  }

  log("succeed to read status line");

  txn_context->m_response.m_status_line = txn_context->m_read_buffer.substr(
      0, bytes_transferred - NEW_LINE.length());
  txn_context->m_read_buffer.erase(0, bytes_transferred);

  const auto handler = [txn_context](const boost::system::error_code& error,
                                     std::size_t bytes_transferred) {
    on_receive_response_header(error, bytes_transferred, txn_context);
  };
  const auto delimiter = NEW_LINE + NEW_LINE;
  boost::asio::async_read_until(
      *txn_context->m_socket,
      boost::asio::dynamic_buffer(txn_context->m_read_buffer), delimiter,
      handler);

  return;
}

void on_receive_response_header(const boost::system::error_code& error,
                                std::size_t bytes_transferred,
                                transaction_context_ptr txn_context) {
  if (error) {
    txn_context->m_completion_callback(error, txn_context->m_response);
    return;
  }

  log("succeed to read header_block");

  const auto delimiter = NEW_LINE + NEW_LINE;
  const auto header_block_str = txn_context->m_read_buffer.substr(
      0, bytes_transferred - delimiter.length());
  txn_context->m_read_buffer.erase(0, bytes_transferred);

  auto& header_block = txn_context->m_response.m_header_block;
  header_block = parse(header_block_str);

  const auto content_length_iter = header_block.find("Content-Length");
  if (content_length_iter == header_block.end()) {
    txn_context->m_completion_callback(error, txn_context->m_response);
    return;
  }

  const auto content_length = std::stoi(content_length_iter->second);
  if (content_length <= 0) {
    txn_context->m_completion_callback(error, txn_context->m_response);
    return;
  }

  const auto remain_length =
      content_length - txn_context->m_read_buffer.length();
  const auto handler = [txn_context](const boost::system::error_code& error,
                                     std::size_t bytes_transferred) {
    on_receive_response_body(error, bytes_transferred, txn_context);
  };
  boost::asio::async_read(
      *txn_context->m_socket,
      boost::asio::dynamic_buffer(txn_context->m_read_buffer),
      boost::asio::transfer_exactly(remain_length), handler);

  return;
}

void on_receive_response_body(const boost::system::error_code& error,
                              std::size_t bytes_transferred,
                              transaction_context_ptr txn_context) {
  if (error) {
    txn_context->m_completion_callback(error, txn_context->m_response);
    return;
  }

  log("succeed to read body");

  txn_context->m_response.m_body = txn_context->m_read_buffer;
  txn_context->m_read_buffer.clear();

  txn_context->m_completion_callback(error, txn_context->m_response);

  return;
}

}  // namespace

http_client::http_client(const std::string& hostname, const uint16_t port)
    : m_io_context{},
      m_executor_work_guard{m_io_context.get_executor()},
      m_future_work{},
      m_hostname{hostname},
      m_socket{construct_connected_socket(&m_io_context, hostname, port)} {
  const auto event_loop_work = [this]() {
    log("start event loop");
    m_io_context.run();
    log("finish event loop");
  };
  m_future_work = std::async(std::launch::async, event_loop_work);
}

http_client::~http_client() {
  try {
    m_executor_work_guard.reset();
    m_future_work.get();
  } catch (const std::exception& e) {
    std::cerr << e.what();
  }
}

void http_client::async_get(const std::string& path,
                            const header_block_t& header_block,
                            callback_t callback) {
  auto copied_header_block{header_block};
  copied_header_block.emplace("Host", m_hostname);
  const http_request request{"GET " + path + " HTTP/1.1", copied_header_block,
                             ""};
  const auto serialized_request = serialize(request);

  auto txn_context = std::make_shared<transaction_context>(m_socket, callback);
  const auto handler = [txn_context](const boost::system::error_code& error,
                                     std::size_t bytes_transferred) {
    on_send_request(error, bytes_transferred, txn_context);
  };
  boost::asio::async_write(*m_socket,
                           boost::asio::buffer(serialized_request.c_str(),
                                               serialized_request.length()),
                           handler);

  return;
}
