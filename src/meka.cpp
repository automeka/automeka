#include <meka.hpp>

#include <fstream>

namespace meka {

  void main(std::string const& program, std::vector< std::string > const& arguments) {
    namespace bfs = ::boost::filesystem;

    if (!bfs::exists("build"))
      bfs::create_directory("build");

    if (!bfs::exists("build/build.ninja"))
      meka::configure(package::root);

    meka::build(package::root);
  }

  meka::package self = {
    path    = meka::root_path,
    name    = "meka",
    version = "0.0.1",

    meka::meka,
  };

}
