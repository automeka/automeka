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

  meka::module_type package::root;

  void main(std::string const& program, std::vector< std::string > const& arguments) {
    cfg::init(program, arguments.empty() ? std::vector< std::string > { "build" } : arguments);

    std::string const command = cfg::get("program.arguments.#0");

    // meka::indent(package::root);
    if (command == "indent")
      return;

    meka::configure(package::root);

    if (command == "configure")
      return;

    meka::build(package::root);

    if (command == "build")
      return;

    // meka::test(package::root);

    if (command == "test")
      return;

    // meka::install(package::root);

    if (command == "install")
      return;

    // meka::pack(package::root);
    if (command == "package")
      return;
  }

}

extern "C" int main(int argc, char const* argv[]) {
  std::string const program = argv[0];

  std::vector< std::string > const arguments(argv + 1, argv + argc);

  meka::main(program, arguments);

  return 0;
}
