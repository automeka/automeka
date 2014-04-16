/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef __MEKA_PACKAGE_HPP__
#define __MEKA_PACKAGE_HPP__

#include <vector>
#include <unordered_map>
#include <functional>

#include <boost/filesystem/path.hpp>

#include "meka/manip/manipulable.hpp"

namespace meka {
  namespace bfs = boost::filesystem;

  struct package_type;

  struct module_type {
    module_type() = default;
    module_type(meka::package_type const& package) :
      package{&package}
    {}

    operator meka::package_type const&() const { return *this->package; }

    meka::package_type const* package = nullptr;
  };

  struct bin_type {
    std::string              name;
    std::vector<std::string> sources;
    std::vector<std::string> links;
  };

  struct lib_type {
    std::string              name;
    std::vector<std::string> sources;
    std::vector<std::string> links;
    std::string              linkage = "shared";
  };

  struct package_type {
    static std::vector<meka::module_type> list;
    static meka::module_type              root();

    package_type() { package_type::list.emplace_back(*this); }

    bfs::path   path;
    std::string name;
    std::string version;

    std::vector<meka::module_type> modules;

    std::vector<meka::bin_type> bins;
    std::vector<meka::lib_type> libs;
  };

  typedef meka::manipulable<package_type> package;

}

#endif // ifndef __MEKA_PACKAGE_HPP__
