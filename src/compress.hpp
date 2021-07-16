// Copyright (C) 2019-2021 Joel Rosdahl and other contributors
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

#include "Util.hpp"

#include <optional>

class Config;
class Context;

void compress_stats(const Config& config,
                    const Util::ProgressReceiver& progress_receiver);

// Recompress the cache.
//
// Arguments:
// - ctx: The context.
// - level: Target compression level (positive or negative value for actual
//   level, 0 for default level and std::nullopt for no compression).
// - progress_receiver: Function that will be called for progress updates.
void compress_recompress(Context& ctx,
                         std::optional<int8_t> level,
                         const Util::ProgressReceiver& progress_receiver);
