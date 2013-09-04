/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#include "meka.hpp"

#include "meka/action/indent.hpp"
#include "meka/action/configure.hpp"
#include "meka/action/build.hpp"
#include "meka/action/test.hpp"
#include "meka/action/install.hpp"
#include "meka/action/pack.hpp"

#include "corefungi.hpp"

namespace meka {
  namespace cfg = ::corefungi;

  meka::module_type package::root;

  // static cfg::sprout const o = {
  //   "Meka options", {
  //     { "meka.prefix","Change installation prefix", cfg::long_name = "prefix", cfg::default_ = "/usr/local" }
  //   }
  // };

  void main(std::string const& program, std::vector< std::string > const& arguments) {
    cfg::init(program, arguments.empty() ? std::vector< std::string > { "build" } : arguments);

    std::string const command = cfg::get("program.arguments.#0");

    if (command == "indent")
      meka::indent(package::root);
    else if (command == "configure")
      meka::configure(package::root);
    else if (command == "build")
      meka::build(package::root);
    else if (command == "test")
      meka::test(package::root);
    else if (command == "install")
      meka::install(package::root);
    else if (command == "package")
      meka::pack(package::root);
  }

}

extern "C" int main(int argc, char const* argv[]) {
  std::string const program = argv[0];

  std::vector< std::string > const arguments(argv + 1, argv + argc);

  meka::main(program, arguments);

  return 0;
}
