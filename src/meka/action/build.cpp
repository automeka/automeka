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

#include <unordered_map>
#include <fstream>

namespace meka {

  namespace bfs = ::boost::filesystem;

  bfs::path make_relative(bfs::path const& from, bfs::path const& to) {
    bfs::path::iterator ifrom = from.begin();
    bfs::path::iterator ito   = to.begin();

    if (*ifrom != *ito)
      return to;

    while ((ifrom != from.end()) && (ito != to.end()) && (*ifrom == *ito)) {
      ++ifrom;
      ++ito;
    }

    bfs::path rel;
    for (; ifrom != from.end(); ++ifrom) {
      rel /= "..";
    }

    for (; ito != to.end(); ++ito) {
      rel /= *ito;
    }

    return rel;
  }

  void build(meka::package const& package) {
    std::clog << "Computing dependencies..." << std::endl;

    std::unordered_map< std::string, std::string > rules;

    auto const& addlib = [&](std::string const& libname) {
                           rules.emplace(libname, "build $builddir/" + libname + "$libext: lib");
                         };
    auto const& addbin = [&](std::string const& binname) {
                           rules.emplace(binname, "build $builddir/" + binname + "$exeext: exe");
                         };
    auto const& addobj = [&](std::string const& binname, std::string const& objname, std::string const& source) {
                           std::string const& target = " $builddir/" + objname + "$objext";
                           rules.emplace(objname, "build" + target + ": cxx " + source);
                           rules[binname] += target;
                         };
    auto const& addbinlinks = [&](std::string const& binname, std::vector< std::string > const& links) {
                                rules[binname] += "\n  libs =";
                                for (auto const& link : links) {
                                  rules[binname] += " -l" + link;
                                }
                              };

    for (bfs::recursive_directory_iterator fit { bfs::current_path() }, end; fit != end; ++fit) {
      bfs::path const&  path  = make_relative(bfs::current_path(), static_cast< bfs::path >(*fit));
      std::string const spath = path.string();
      std::string const stem  = (path.parent_path() / path.stem()).string();

      for (auto const& bin : package.bins) {
        addbin("bin/" + bin.name);

        for (auto const& src : bin.sources) {
          if (boost::regex_match(std::begin(spath), std::end(spath), boost::regex { src }))
            addobj("bin/" + bin.name, stem, spath);
        }
      }

      for (auto const& lib : package.libs) {
        addlib("lib/${libprefix}" + lib.name);

        for (auto const& src : lib.sources) {
          if (boost::regex_match(std::begin(spath), std::end(spath), boost::regex { src }))
            addobj("lib/${libprefix}" + lib.name, stem, spath);
        }
      }
    }

    for (auto const& bin : package.bins) {
      rules["bin/" + bin.name] += " |";
      for (auto const& lib : package.libs) {
        rules["bin/" + bin.name] += " $builddir/lib/${libprefix}" + lib.name + "$libext";
      }
    }

    for (auto const& bin : package.bins) {
      addbinlinks("bin/" + bin.name, bin.links);
    }

    std::ofstream ninja { "build.ninja" };
    ninja << "cblk = [30m\n";
    ninja << "cred = [31m\n";
    ninja << "cgrn = [32m\n";
    ninja << "cylw = [33m\n";
    ninja << "cblu = [34m\n";
    ninja << "cprp = [35m\n";
    ninja << "ccyn = [36m\n";
    ninja << "cwht = [37m\n";
    ninja << "crgb = [38m\n";
    ninja << "cdef = [39m\n";
    ninja << "crst = [0m\n";
    ninja << "\n";
    ninja << "cbblk = ${cblk}[1m\n";
    ninja << "cbred = ${cred}[1m\n";
    ninja << "cbgrn = ${cgrn}[1m\n";
    ninja << "cbylw = ${cylw}[1m\n";
    ninja << "cbblu = ${cblu}[1m\n";
    ninja << "cbprp = ${cprp}[1m\n";
    ninja << "cbcyn = ${ccyn}[1m\n";
    ninja << "cbwht = ${cwht}[1m\n";
    ninja << "cbrgb = ${crgb}[1m\n";
    ninja << "cbdef = ${cdef}[1m\n";
    ninja << "cbrst = ${crst}[1m\n";
    ninja << "\n";
    ninja << "\n";
    ninja << "builddir = build/host/\n";
    ninja << "cxx = clang++\n";
    ninja << "ar  = ar\n";
    ninja << "cflags  = -std=c++11 -fpic -Iinclude -Qunused-arguments\n";
    ninja << "ldflags = -L$builddir/lib -Wl,-rpath,$builddir/lib\n";
    ninja << "\n";
    ninja << "libprefix = lib\n";
    ninja << "exeext    =\n";
    ninja << "libext    = .so\n";
    ninja << "objext    = .o\n";
    ninja << "\n";
    ninja << "rule cxx\n";
    ninja << "  command = $cxx -MMD -MT $out -MF $out.d $cflags -xc++ -c $in -o $out\n";
    ninja << "  description = ${cylw}CXX${crst} ${cgrn}$out${crst}\n";
    ninja << "  depfile = $out.d\n";
    ninja << "  deps = gcc\n";
    ninja << "\n";
    ninja << "rule ar\n";
    ninja << "  command = rm -f $out && $ar crs $out $in\n";
    ninja << "  description = ${cylw}AR${crst}  ${cblu}$out${crst}\n";
    ninja << "\n";
    ninja << "rule lib\n";
    ninja << "  command = $cxx -shared $cflags -o $out $in\n";
    ninja << "  description = ${cylw}LIB${crst} ${cblu}$out${crst}\n";
    ninja << "\n";
    ninja << "rule exe\n";
    ninja << "  command = $cxx $ldflags -o $out $in $libs\n";
    ninja << "  description = ${cylw}EXE${crst} ${cblu}$out${crst}\n";
    ninja << "\n";
    ninja << "# rule configure\n";
    ninja << "#   command = ${configure_env}python configure.py $configure_args\n";
    ninja << "#   generator = 1\n";
    ninja << "# build build.ninja: configure | configure.py misc/ninja_syntax.py\n";
    ninja << std::endl;

    for (auto const& pair : rules) {
      ninja << pair.second << std::endl;
    }

    std::clog << "Building package: " << package.name << ", version: " << package.version << " from: " << package.path << "..." << std::endl;
    std::system("ninja");
  } // build

}
