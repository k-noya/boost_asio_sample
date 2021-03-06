// Copyright (c) 2021, k-noya
// Distributed under the BSD 3-Clause License.
// See accompanying file LICENSE
#ifndef SIMPLE_ASYNC_HTTP_CLIENT_HTTP_H_
#define SIMPLE_ASYNC_HTTP_CLIENT_HTTP_H_

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
const auto HEADER_BLOCK_DELIMITER = NEW_LINE + NEW_LINE;

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

#endif  // SIMPLE_ASYNC_HTTP_CLIENT_HTTP_H_
