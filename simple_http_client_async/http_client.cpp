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

boost::asio::ip::tcp::socket construct_connected_socket(
    boost::asio::io_context* io_context_ptr, const std::string& hostname,
    const uint16_t port) {
  boost::asio::ip::tcp::resolver resolver{*io_context_ptr};
  const auto results = resolver.resolve(hostname, std::to_string(port));
  const auto remote_endpoint = results.begin()->endpoint();

  auto socket = boost::asio::ip::tcp::socket(*io_context_ptr);
  socket.connect(remote_endpoint);
  return socket;
}

}  // namespace

http_client::http_client(const std::string& hostname, const uint16_t port)
    : m_io_context{},
      m_executor_work_guard{m_io_context.get_executor()},
      m_hostname{hostname},
      m_socket{construct_connected_socket(&m_io_context, hostname, port)},
      m_work_future{},
      m_transaction_map{},
      m_next_id{},
      m_transaction_mutex{} {
  const auto event_loop_work = [this]() {
    log("start event loop");
    m_io_context.run();
    log("finish event loop");
  };
  m_work_future = std::async(std::launch::async, event_loop_work);
}

http_client::~http_client() {
  try {
    m_executor_work_guard.reset();
    m_work_future.get();
  } catch (const std::exception& e) {
    std::cerr << e.what();
  }
}

void http_client::get_async(const std::string& path,
                            const header_block_t& header_block,
                            callback_t callback) {
  const auto transaction_id = register_new_transaction(callback);

  auto copied_header_block{header_block};
  copied_header_block.emplace("Host", m_hostname);
  const http_request request{"GET " + path + " HTTP/1.1", copied_header_block,
                             ""};
  const auto serialized_request = serialize(request);

  const auto handler = [this, transaction_id](
                           const boost::system::error_code& error,
                           std::size_t bytes_transferred) {
    on_send_request(error, bytes_transferred, transaction_id);
  };
  boost::asio::async_write(m_socket,
                           boost::asio::buffer(serialized_request.c_str(),
                                               serialized_request.length()),
                           handler);

  return;
}

uint64_t http_client::register_new_transaction(const callback_t& callback) {
  std::lock_guard lock{m_transaction_mutex};
  const auto transaction_id = m_next_id;
  m_transaction_map[transaction_id] =
      std::make_shared<http_transaction_context>(callback);
  ++m_next_id;
  return transaction_id;
}

void http_client::remove_transaction(uint64_t transaction_id) {
  std::lock_guard lock{m_transaction_mutex};
  m_transaction_map.erase(transaction_id);
}

void http_client::on_send_request(const boost::system::error_code& error,
                                  std::size_t bytes_transferred,
                                  const uint64_t transaction_id) {
  auto transaction_context_ptr = m_transaction_map.at(transaction_id);
  if (error) {
    transaction_context_ptr->m_completion_callback(
        error, transaction_context_ptr->m_response);
    return;
  }

  log("succeed to request");

  transaction_context_ptr->m_read_buffer.clear();

  const auto handler = [this, transaction_id](
                           const boost::system::error_code& error,
                           std::size_t bytes_transferred) {
    on_receive_status_line(error, bytes_transferred, transaction_id);
  };
  const auto delimiter = NEW_LINE;
  boost::asio::async_read_until(
      m_socket,
      boost::asio::dynamic_buffer(transaction_context_ptr->m_read_buffer),
      delimiter, handler);

  return;
}

void http_client::on_receive_status_line(const boost::system::error_code& error,
                                         std::size_t bytes_transferred,
                                         const uint64_t transaction_id) {
  auto transaction_context_ptr = m_transaction_map.at(transaction_id);
  if (error) {
    transaction_context_ptr->m_completion_callback(
        error, transaction_context_ptr->m_response);
    return;
  }

  log("succeed to read status line");

  transaction_context_ptr->m_response.m_status_line =
      transaction_context_ptr->m_read_buffer.substr(
          0, bytes_transferred - NEW_LINE.length());
  transaction_context_ptr->m_read_buffer.erase(0, bytes_transferred);

  const auto handler = [this, transaction_id](
                           const boost::system::error_code& error,
                           std::size_t bytes_transferred) {
    on_receive_response_header(error, bytes_transferred, transaction_id);
  };
  const auto delimiter = NEW_LINE + NEW_LINE;
  boost::asio::async_read_until(
      m_socket,
      boost::asio::dynamic_buffer(transaction_context_ptr->m_read_buffer),
      delimiter, handler);

  return;
}

void http_client::on_receive_response_header(
    const boost::system::error_code& error, std::size_t bytes_transferred,
    const uint64_t transaction_id) {
  auto transaction_context_ptr = m_transaction_map.at(transaction_id);
  if (error) {
    transaction_context_ptr->m_completion_callback(
        error, transaction_context_ptr->m_response);
    return;
  }

  log("succeed to read header_block");

  const auto delimiter = NEW_LINE + NEW_LINE;
  const auto header_block_str = transaction_context_ptr->m_read_buffer.substr(
      0, bytes_transferred - delimiter.length());
  transaction_context_ptr->m_read_buffer.erase(0, bytes_transferred);

  auto& header_block = transaction_context_ptr->m_response.m_header_block;
  header_block = parse(header_block_str);

  const auto content_length_iter = header_block.find("Content-Length");
  if (content_length_iter == header_block.end()) {
    transaction_context_ptr->m_completion_callback(
        error, transaction_context_ptr->m_response);
    return;
  }

  const auto content_length = std::stoi(content_length_iter->second);
  if (content_length <= 0) {
    transaction_context_ptr->m_completion_callback(
        error, transaction_context_ptr->m_response);
    return;
  }

  const auto remain_length =
      content_length - transaction_context_ptr->m_read_buffer.length();
  const auto handler = [this, transaction_id](
                           const boost::system::error_code& error,
                           std::size_t bytes_transferred) {
    on_receive_response_body(error, bytes_transferred, transaction_id);
  };
  boost::asio::async_read(
      m_socket,
      boost::asio::dynamic_buffer(transaction_context_ptr->m_read_buffer),
      boost::asio::transfer_exactly(remain_length), handler);

  return;
}

void http_client::on_receive_response_body(
    const boost::system::error_code& error, std::size_t bytes_transferred,
    const uint64_t transaction_id) {
  auto transaction_context_ptr = m_transaction_map.at(transaction_id);
  if (error) {
    transaction_context_ptr->m_completion_callback(
        error, transaction_context_ptr->m_response);
    return;
  }

  log("succeed to read body");

  transaction_context_ptr->m_response.m_body =
      transaction_context_ptr->m_read_buffer;
  transaction_context_ptr->m_read_buffer.clear();

  transaction_context_ptr->m_completion_callback(
      error, transaction_context_ptr->m_response);

  remove_transaction(transaction_id);

  return;
}
