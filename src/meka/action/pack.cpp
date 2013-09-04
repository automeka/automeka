/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#include "meka/action/pack.hpp"

#include "meka/action/configure.hpp"
#include "meka/action/build.hpp"
#include "meka/package.hpp"

#include <boost/filesystem.hpp>

namespace meka {

  void pack(meka::package const& package) {
    if (!bfs::exists("build/package.ninja"))
      meka::configure(package);

    meka::build(package);

    std::system("${NINJA:-ninja} -f build/package.ninja");
  }

}
