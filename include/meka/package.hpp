/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef __MEKA_PACKAGE_HPP__
#define __MEKA_PACKAGE_HPP__

#include <vector>
#include <functional>

#include <boost/filesystem/path.hpp>

namespace meka {
  namespace bfs = boost::filesystem;

  struct package;

  struct module_type {
    module_type(meka::package const& package) :
      package{&package}
    {}

    operator meka::package const&() const { return *this->package; }

    meka::package const* package;
  };

  struct bin_type {
    typedef std::function< void (meka::bin_type&) > manipulator;
    typedef std::vector< manipulator >              manipulators;

    template< typename ... Ts >
    bin_type(Ts&& ... ts) {
      for (auto const& manipulate : bin_type::manipulators { std::forward< Ts >(ts) ... }) {
        manipulate(*this);
      }
    }

    std::string                name;
    std::vector< std::string > sources;
    std::vector< std::string > links;
  };

  struct lib_type {
    typedef std::function< void (meka::lib_type&) > manipulator;
    typedef std::vector< manipulator >              manipulators;

    template< typename ... Ts >
    lib_type(Ts&& ... ts) {
      for (auto const& manipulate : lib_type::manipulators { std::forward< Ts >(ts) ... }) {
        manipulate(*this);
      }
    }

    std::string                name;
    std::vector< std::string > sources;
    std::vector< std::string > links;
  };

  struct package {
    typedef std::function< void (meka::package&) > manipulator;
    typedef std::vector< manipulator >             manipulators;

    template< typename ... Ts >
    package(Ts&& ... ts) {
      for (auto const& manipulate : package::manipulators { std::forward< Ts >(ts) ... }) {
        manipulate(*this);
      }
    }

    bfs::path   path;
    std::string name;
    std::string version;

    std::vector< meka::module_type > modules;

    std::vector< meka::bin_type > bins;
    std::vector< meka::lib_type > libs;
  };

}

#endif // ifndef __MEKA_PACKAGE_HPP__
