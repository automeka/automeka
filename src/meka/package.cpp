/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#include "meka/package.hpp"

namespace meka {

  meka::module_type meka::package_type::root() {
    auto const& it = std::find_if(std::begin(package_type::list), std::end(package_type::list),
                                  [](meka::module_type const& module) {
                                    return static_cast< meka::package_type const& >(module).path.string().empty();
                                  });

    if (it == std::end(package_type::list))
      return {};
    return *it;
  }

}
