// Copyright (c) 2021, k-noya
// Distributed under the BSD 3-Clause License.
// See accompanying file LICENSE
#ifndef SIMPLE_HTTP_CLIENT_ASYNC_HTTP_H_
#define SIMPLE_HTTP_CLIENT_ASYNC_HTTP_H_

#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include "boost/asio/ip/tcp.hpp"
#include "boost/system/error_code.hpp"

// Common
using header_t = std::pair<std::string, std::string>;
using header_block_t = std::map<std::string, std::string>;

const auto NEW_LINE = std::string{"\r\n"};

// HTTP request
struct http_request {
  std::string m_request_line;
  header_block_t m_header_block;
  std::string m_body;
};

std::string serialize(const http_request& request);

// HTTP response
struct http_response {
  std::string m_status_line;
  header_block_t m_header_block;
  std::string m_body;
};

header_block_t parse(const std::string& header_block_str);
std::ostream& operator<<(std::ostream& out, const http_response& resopnse);

// HTTP transaction
using callback_t =
    std::function<void(const boost::system::error_code&, const http_response)>;

struct http_transaction_context {
  explicit http_transaction_context(const callback_t& completion_callback);
  http_response m_response;
  std::string m_read_buffer;
  callback_t m_completion_callback;
};

using http_transaction_context_ptr = std::shared_ptr<http_transaction_context>;

#endif  // SIMPLE_HTTP_CLIENT_ASYNC_HTTP_H_
