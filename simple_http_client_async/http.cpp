// Copyright (c) 2021, k-noya
// Distributed under the BSD 3-Clause License.
// See accompanying file LICENSE
#include "simple_http_client_async/http.h"

#include <algorithm>
#include <ostream>
#include <string>
#include <vector>

#include "boost/algorithm/string/classification.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/trim.hpp"

std::string serialize(const http_request& request) {
  auto serialized_request{request.m_request_line + NEW_LINE};

  std::for_each(request.m_header_block.begin(), request.m_header_block.end(),
                [&serialized_request](const auto& header) {
                  serialized_request +=
                      header.first + ": " + header.second + NEW_LINE;
                });

  if (request.m_body.length() > 0) {
    serialized_request += request.m_body;
  }

  serialized_request += NEW_LINE;

  return serialized_request;
}

header_block_t parse(const std::string& header_block_str) {
  std::vector<std::string> header_lines;
  boost::algorithm::split(header_lines, header_block_str,
                          boost::is_any_of(NEW_LINE),
                          boost::algorithm::token_compress_on);

  header_block_t header_block;
  std::for_each(
      header_lines.begin(), header_lines.end(),
      [&header_block](const auto header) {
        const auto index = header.find_first_of(':');
        const auto header_name =
            boost::algorithm::trim_right_copy(header.substr(0, index));
        const auto header_value =
            boost::algorithm::trim_left_copy(header.substr(index + 1));
        header_block.insert({header_name, header_value});
      });

  return header_block;
}

std::ostream& operator<<(std::ostream& out, const http_response& response) {
  out << '\n';
  out << "[status line]\n";
  out << response.m_status_line << '\n';
  out << '\n';
  out << "[header block]\n";
  std::for_each(response.m_header_block.begin(), response.m_header_block.end(),
                [&out](const auto& header) {
                  out << header.first << ": " << header.second << '\n';
                });
  out << '\n';
  out << "[body]\n";
  out << response.m_body;

  return out;
}
