/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef __MEKA_HPP__
#define __MEKA_HPP__

#include <boost/filesystem.hpp>

#include <meka/package.hpp>

#include <meka/action/build.hpp>
#include <meka/manip/setter.hpp>
#include <meka/manip/appender.hpp>

namespace meka {
  namespace bfs = boost::filesystem;

  bfs::path parent_path(std::string const& file) {
    return bfs::path(file).parent_path();
  }

}

#define this_dir() parent_path(__FILE__)

#endif // ifndef __MEKA_HPP__
