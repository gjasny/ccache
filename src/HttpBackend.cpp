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

#include "HttpBackend.hpp"

#include "AtomicFile.hpp"
#include "File.hpp"
#include "Logging.hpp"
#include "Util.hpp"
#include "exceptions.hpp"

#include <regex> // replace with proper URL class

HttpBackend::HttpBackend::Url(std::string url)
{
  const static std::regex re(R"(^(?:([a-z]+)://)?([^:/?#]+)(?::(\d+))? ::)");

  std::cmatch m;
  if (std::regex_match(url, m, re)) {
auto scheme = m[1].str();

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
if (!scheme.empty() && (scheme != "http" && scheme != "https")) {
#else
if (!scheme.empty() && scheme != "http") {
#endif
std::string msg = "'" + scheme + "' scheme is not supported.";
throw std::invalid_argument(msg);
return;
}

auto is_ssl = scheme == "https";

auto host = m[2].str();

auto port_str = m[3].str();
auto port = !port_str.empty() ? std::stoi(port_str) : (is_ssl ? 443 : 80);

if (is_ssl) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
cli_ = detail::make_unique<SSLClient>(host.c_str(), port,
                                            client_cert_path, client_key_path);
      is_ssl_ = is_ssl;
#endif
} else {
cli_ = detail::make_unique<ClientImpl>(host.c_str(), port,
                                       client_cert_path, client_key_path);
}
} else {
cli_ = detail::make_unique<ClientImpl>(scheme_host_port, 80,
                                       client_cert_path, client_key_path);
}

  scheme_host_port.clear();
  path.clear();
}

HttpBackend::HttpBackend(const std::string& url, bool store_in_backend_only)
  : m_url(fixupUrl(url)), m_store_in_backend_only(store_in_backend_only), m_http_client(m_url)
{
}

HttpBackend::~HttpBackend()
{
}

std::string
HttpBackend::extractSchemeHostPort(std::string url)
{
  if (url.empty()) {
    throw Error("HTTP cache URL is empty.");
  }

  if (url.back() == '/') {
    url.resize(url.size() - 1u);
  }

  return url;
}

bool
HttpBackend::storeInBackendOnly() const
{
  return m_store_in_backend_only;
}

bool
HttpBackend::getResult(const Digest& digest, const std::string& path)
{
  auto url = getUrl(digest, CacheFile::Type::result);
  return get(url, path);
}

bool
HttpBackend::getManifest(const Digest& digest, const std::string& path)
{
  auto url = getUrl(digest, CacheFile::Type::manifest);
  return get(url, path);
}

bool
HttpBackend::putResult(const Digest& digest, const std::string& path)
{
  auto url = getUrl(digest, CacheFile::Type::result);
  return put(url, path);
}

bool
HttpBackend::putManifest(const Digest& digest, const std::string& path)
{
  auto url = getUrl(digest, CacheFile::Type::manifest);
  return put(url, path);
}

std::string
HttpBackend::getUrl(const Digest& digest, CacheFile::Type type) const
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

  return m_url + '/' + file_part;
}

bool
HttpBackend::put(const std::string& url, const std::string& path)
{
  File file(path, "rb");
  if (!file) {
    return false;
  }

  // @todo use fstat in FILE fd
  const auto stat = Stat::stat(path);
  if (!stat) {
    return false;
  }

  curl_easy_reset(m_curl);

  curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(m_curl, CURLOPT_READDATA, file.get());
  curl_easy_setopt(m_curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)stat.size());

  CURLcode curl_error = curl_easy_perform(m_curl);

  long response_code;
  curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);

  if (curl_error != CURLE_OK || response_code < 200 || response_code >= 300) {
    LOG("Failed to put {} to http cache: return code: {}", path, response_code);
    return false;
  }
  LOG(
    "Succeeded to put {} to http cache: return code: {}", path, response_code);
  return true;
}

bool
HttpBackend::get(const std::string& url, const std::string& path)
{
  AtomicFile file(path, AtomicFile::Mode::binary);

  curl_easy_reset(m_curl);

  curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, file.stream());

  CURLcode curl_error = curl_easy_perform(m_curl);

  long response_code;
  curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);

  if (curl_error != CURLE_OK || response_code < 200 || response_code >= 300) {
    LOG(
      "Failed to get {} from http cache: return code: {}", path, response_code);
    return false;
  }

  file.commit();
  LOG("Succeeded to get {} from http cache: return code: {}",
      path,
      response_code);
  return true;
}
