#include <meka.hpp>

#include <fstream>

namespace meka {
  extern meka::package self;
}

extern "C" int main(int argc, char const* argv[]) {
  namespace bfs = ::boost::filesystem;

  if (!bfs::exists("build"))
    bfs::create_directory("build");

  if (!bfs::exists("build/build.ninja"))
    meka::configure(meka::self);

  meka::build(meka::self);

  return 0;
}

namespace meka {

  meka::package self = {
    path    = meka::parent_path(meka::this_dir().string()),
    name    = "meka",
    version = "0.0.1",

    meka::meka,
  };

}
