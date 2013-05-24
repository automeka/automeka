/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#include "meka.hpp"

#include "corefungi.hpp"

namespace meka {
  namespace cfg = ::corefungi;

  extern meka::package self;

  void main(std::string const& program, std::vector< std::string > const& arguments) {
    cfg::init(program, arguments.empty() ? std::vector< std::string > { "build" } : arguments);

    std::string const command = cfg::get("program.arguments.#0");

    // meka::indent(meka::self);
    if (command == "indent")
      return;

    meka::configure(meka::self);

    if (command == "configure")
      return;

    meka::build(meka::self);

    if (command == "build")
      return;

    // meka::test(meka::self);

    if (command == "test")
      return;

    // meka::install(meka::self);

    if (command == "install")
      return;

    // meka::pack(meka::self);
    if (command == "package")
      return;
  }

}
