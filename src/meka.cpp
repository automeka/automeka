#include <meka.hpp>

#include <meka/main>

namespace meka {

  meka::package self = {
    path    = meka::this_dir() / "..",
    name    = "meka",
    version = "0.0.1",

    meka::meka,
  };

}
