/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef __MEKA_PATH_HPP__
#define __MEKA_PATH_HPP__

#include <boost/filesystem.hpp>
#include <boost/range/algorithm/mismatch.hpp>

namespace meka {
  namespace bfs = ::boost::filesystem;

  static inline bfs::path relative_path(bfs::path const& from, bfs::path const& to) {
    auto const& pair = boost::mismatch(from, to);

    if (pair.first == std::begin(from))
      return to;

    auto rel = bfs::path {};
    std::for_each(pair.first, from.end(), [&](bfs::path const& i) { rel /= ".."; });
    std::for_each(pair.second, to.end(), [&](bfs::path const& i) { rel /= i; });

    return rel;
  }

  static inline bfs::path parent_path(std::string const& file) {
    return meka::relative_path(bfs::current_path(), bfs::path(file).parent_path());
  }

#define this_dir() parent_path(__FILE__)

}

#endif // ifndef __MEKA_PATH_HPP__
