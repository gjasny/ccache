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

#pragma once

#include "CacheFile.hpp"
#include "Digest.hpp"
#include "StorageBackend.hpp"

#include <memory>
#include <string>

namespace httplib {
class Client;
}

// A decompressor of an uncompressed stream.
class HttpBackend : public StorageBackend
{
public:
  HttpBackend(const std::string& url, bool store_in_backend_only);

  ~HttpBackend() override;

  bool storeInBackendOnly() const override;

  bool getResult(const Digest& digest, const std::string& file_path) override;
  bool getManifest(const Digest& digest, const std::string& file_path) override;

  bool putResult(const Digest& digest, const std::string& file_path) override;
  bool putManifest(const Digest& digest, const std::string& file_path) override;

private:
  std::string getUrlPath(const Digest& digest, CacheFile::Type type) const;
  bool get(const std::string& url_path, const std::string& file_path);
  bool put(const std::string& url_path, const std::string& file_path);

  struct Url
  {
    Url(std::string url);

    std::string scheme_host_port;
    std::string path;
  };
  const Url m_url;
  const bool m_store_in_backend_only;
  std::unique_ptr<httplib::Client> m_http_client;
};
