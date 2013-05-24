/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef __MEKA_HPP__
#define __MEKA_HPP__

#include <meka/package.hpp>
#include <meka/path.hpp>

#include <meka/action/configure.hpp>
#include <meka/action/build.hpp>

#include <meka/manip/setter.hpp>
#include <meka/manip/appender.hpp>

namespace meka {
  namespace bfs = boost::filesystem;

  static meka::bin const meka {
    name    = "../meka",
    sources = { "meka" },
    links   = { "meka", "boost_filesystem", "boost_system", "boost_regex" }

    // meka::host
  };

}

#endif // ifndef __MEKA_HPP__
