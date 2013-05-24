/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#include "meka/action/build.hpp"

#include "meka/package.hpp"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/join.hpp>

#include <unordered_map>
#include <fstream>

namespace meka {

  void build(meka::package const& package) {
    std::system("${NINJA:-ninja} -f build/build.ninja");
  }

}
