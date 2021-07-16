// Copyright (C) 2021 Joel Rosdahl and other contributors
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

#include <Counters.hpp>
#include <Digest.hpp>
#include <core/types.hpp>
#include <storage/types.hpp>

#include <optional>

class Config;
class Counters;

namespace storage {
namespace primary {

class PrimaryStorage
{
public:
  PrimaryStorage(const Config& config);

  void initialize();
  void finalize();

  // Returns a path to a file containing the value.
  std::optional<std::string> get(const Digest& key,
                                    core::CacheEntryType type) const;

  std::optional<std::string>
  put(const Digest& key,
      core::CacheEntryType type,
      const storage::CacheEntryWriter& entry_writer);

  void remove(const Digest& key, core::CacheEntryType type);

  void increment_statistic(Statistic statistic, int64_t value = 1);

  // Return a machine-readable string representing the final ccache result, or
  // nullopt if there was no result.
  std::optional<std::string> get_result_id() const;

  // Return a human-readable string representing the final ccache result, or
  // nullopt if there was no result.
  std::optional<std::string> get_result_message() const;

private:
  const Config& m_config;

  // Main statistics updates (result statistics and size/count change for result
  // file) which get written into the statistics file belonging to the result
  // file.
  Counters m_result_counter_updates;

  // Statistics updates (only for manifest size/count change) which get written
  // into the statistics file belonging to the manifest.
  Counters m_manifest_counter_updates;

  // The manifest and result keys and paths are stored by put() so that
  // finalize() can use them to move the files in place.
  std::optional<Digest> m_manifest_key;
  std::optional<Digest> m_result_key;
  std::string m_manifest_path;
  std::string m_result_path;

  struct LookUpCacheFileResult
  {
    std::string path;
    Stat stat;
    uint8_t level;
  };

  LookUpCacheFileResult look_up_cache_file(const Digest& key,
                                           core::CacheEntryType type) const;

  void clean_up_internal_tempdir();

  std::optional<Counters>
  update_stats_and_maybe_move_cache_file(const Digest& key,
                                         const std::string& current_path,
                                         const Counters& counter_updates,
                                         core::CacheEntryType type);
};

} // namespace primary
} // namespace storage
