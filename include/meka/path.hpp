/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef __MEKA_PATH_HPP__
#define __MEKA_PATH_HPP__

#include <boost/filesystem.hpp>

namespace meka {
  namespace bfs = ::boost::filesystem;

  static inline bfs::path relative_path(bfs::path const& from, bfs::path const& to) {
    bfs::path::iterator ifrom = from.begin();
    bfs::path::iterator ito   = to.begin();

    if (*ifrom != *ito)
      return to;

    while ((ifrom != from.end()) && (ito != to.end()) && (*ifrom == *ito)) {
      ++ifrom;
      ++ito;
    }

    bfs::path rel;
    for (; ifrom != from.end(); ++ifrom) {
      rel /= "..";
    }

    for (; ito != to.end(); ++ito) {
      rel /= *ito;
    }

    return rel;
  }

  static inline bfs::path parent_path(std::string const& file) {
    return meka::relative_path(bfs::current_path(), bfs::canonical(bfs::path(file)).parent_path());
  }

#define this_dir() parent_path(__FILE__)

}

#endif // ifndef __MEKA_PATH_HPP__
