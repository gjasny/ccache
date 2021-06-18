// Copyright (C) 2020 Joel Rosdahl and other contributors
//
// See doc/AUTHORS.adoc for a complete list of contributors.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "HttpBackend.hpp"

#include "AtomicFile.hpp"
#include "File.hpp"
#include "Logging.hpp"
#include "Util.hpp"
#include "ccache.hpp"
#include "exceptions.hpp"
#include "fmtmacros.hpp"

#include "third_party/httplib.h"

#include <regex> // replace with proper URL class

HttpBackend::Url::Url(std::string url)
{
  const std::regex rfc3986Regex(
    R"(^(([^:\/?#]+):)?(//([^\/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)",
    std::regex::extended);

  std::smatch result;
  if (!std::regex_match(url, result, rfc3986Regex)) {
    throw Error("Invalid URL: {}", url);
  }

  scheme_host_port = result[1].str() + result[3].str();
  path = result[5].str();
  if (path.empty() || path.back() != '/') {
    path += '/';
  }
}

HttpBackend::HttpBackend(const std::string& url, bool store_in_backend_only)
  : m_url(url),
    m_store_in_backend_only(store_in_backend_only),
    m_http_client(std::make_unique<httplib::Client>(m_url.scheme_host_port.c_str()))
{
  m_http_client->set_default_headers(
    {{"User-Agent", FMT("{}/{}", CCACHE_NAME, CCACHE_VERSION)}});
  m_http_client->set_keep_alive(true);
}

bool
HttpBackend::storeInBackendOnly() const
{
  return m_store_in_backend_only;
}

bool
HttpBackend::getResult(const Digest& digest, const std::string& file_path)
{
  auto url_path = getUrlPath(digest, CacheFile::Type::result);
  return get(url_path, file_path);
}

bool
HttpBackend::getManifest(const Digest& digest, const std::string& file_path)
{
  auto url_path = getUrlPath(digest, CacheFile::Type::manifest);
  return get(url_path, file_path);
}

bool
HttpBackend::putResult(const Digest& digest, const std::string& file_path)
{
  auto url_path = getUrlPath(digest, CacheFile::Type::result);
  return put(url_path, file_path);
}

bool
HttpBackend::putManifest(const Digest& digest, const std::string& file_path)
{
  auto url_path = getUrlPath(digest, CacheFile::Type::manifest);
  return put(url_path, file_path);
}

std::string
HttpBackend::getUrlPath(const Digest& digest, CacheFile::Type type) const
{
  auto file_part = digest.to_string();

  switch (type) {
  case CacheFile::Type::result:
    file_part.append(".result");
    break;

  case CacheFile::Type::manifest:
    file_part.append(".manifest");
    break;

  case CacheFile::Type::unknown:
    file_part.append(".unknown");
    break;
  }

  return m_url.path + file_part;
}

bool
HttpBackend::put(const std::string& url_path, const std::string& file_path)
{
  File file(file_path, "rb");
  if (!file) {
    return false;
  }

  // @todo use fstat in FILE fd
  const auto stat = Stat::stat(file_path);
  if (!stat) {
    return false;
  }

  const auto content_length = stat.size();
  const auto content_type = "application/octet-stream";

  const auto content_provider =
    [&file](size_t offset, size_t length, httplib::DataSink& sink) -> bool {
#if defined(_WIN32)
    auto err = ::_fseeki64(*file, offset, SEEK_SET);
#else
    auto err = ::fseeko(*file, offset, SEEK_SET);
#endif
    if (err) {
      return false;
    }

    const size_t buffer_size = 65536;
    char buffer[buffer_size];

    auto bytes_read =
      std::fread(buffer, 1, std::max(length, buffer_size), *file);
    sink.write(buffer, bytes_read);

    return !std::ferror(*file);
  };

  const auto result = m_http_client->Put(
    url_path.c_str(), content_length, content_provider, content_type);

  if (result.error() != httplib::Error::Success || !result) {
    LOG("Failed to put {} to http cache: error code: {}",
        url_path,
        result.error());
    return false;
  }

  if (result->status < 200 || result->status >= 300) {
    LOG("Failed to put {} to http cache: status code: {}",
        url_path,
        result->status);
    return false;
  }
  LOG("Succeeded to put {} to http cache: status code: {}",
      url_path,
      result->status);
  return true;
}

bool
HttpBackend::get(const std::string& url_path, const std::string& file_path)
{
  AtomicFile file(file_path, AtomicFile::Mode::binary);

  const auto content_receiver = [&file](const char* data,
                                        size_t data_length) -> bool {
    auto bytes_written = std::fwrite(data, 1, data_length, file.stream());

    return bytes_written == data_length;
  };

  const auto result = m_http_client->Get(url_path.c_str(), content_receiver);

  if (result.error() != httplib::Error::Success || !result) {
    LOG("Failed to get {} from http cache: error code: {}",
        url_path,
        result.error());
    return false;
  }

  if (result->status < 200 || result->status >= 300) {
    LOG("Failed to get {} from http cache: status code: {}",
        url_path,
        result->status);
    return false;
  }

  file.commit();
  LOG("Succeeded to get {} from http cache: status code: {}",
      url_path,
      result->status);
  return true;
}
