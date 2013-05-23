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
#include <boost/algorithm/string/join.hpp>

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

  static void genrules(meka::package const& package, std::unordered_map< std::string, std::string >& rules) {
    std::clog << "Found package: " << package.name << ", version: " << package.version << " path: " << package.path << "..." << std::endl;

    auto const& objout = [](std::string const& name) { return "${builddir}/obj/" + name + "${objext}"; };
    auto const& libout = [](std::string const& name) { return "${builddir}/lib/${libprefix}" + name + "${libext}"; };
    auto const& exeout = [](std::string const& name) { return "${builddir}/bin/" + name + "${exeext}"; };

    auto const& addbin = [&](std::string const& binname, std::string const& type) {
                           std::string const& binout = type == "lib" ? libout(binname) : exeout(binname);
                           rules.emplace(binout, "build " + binout + ": " + type);
                         };
    auto const& addobj = [&](std::string const& binname, std::string const& objname, std::string const& source, std::string const& incdirs) {
                           rules.emplace(objname, "build " + objname + ": cxx " + source + "\n  incdirs = " + incdirs);
                           rules[binname] += " " + objname;
                         };
    auto const& addlinks = [&](std::string const& binname, std::vector< std::string > const& links) {
                                rules[binname] += "\n  libs =";
                                for (auto const& link : links) {
                                  rules[binname] += " -l" + link;
                                }
                              };

    std::vector< std::string > idirs = {"-I" + (package.path / "src").string(), "-I" + (package.path / "include").string()};
    std::transform(std::begin(package.modules), std::end(package.modules), std::back_inserter(idirs), [](meka::package const& module) { return "-I" + (module.path / "include").string(); });
    std::string const& incdirs = boost::algorithm::join(idirs, " ");

    for(meka::package const& module : package.modules) {
      meka::genrules(module, rules);
    }

    for (bfs::recursive_directory_iterator fit { package.path }, end; fit != end; ++fit) {
      bfs::path const   path  = *fit;
      std::string const spath = make_relative(package.path, path).string();
      std::string const opath = make_relative(bfs::current_path(), path.parent_path() / path.stem()).string();

      for (auto const& bin : package.bins) {
        addbin(bin.name, "exe");

        for (auto const& src : bin.sources) {
          if (boost::regex_match(std::begin(spath), std::end(spath), boost::regex { src }))
            addobj(exeout(bin.name), objout(opath), (package.path / spath).string(), incdirs);
        }
      }
      for (auto const& lib : package.libs) {
        addbin(lib.name, "lib");

        for (auto const& src : lib.sources) {
          if (boost::regex_match(std::begin(spath), std::end(spath), boost::regex { src }))
            addobj(libout(lib.name), objout(opath), (package.path / spath).string(), incdirs);
        }
      }
    }

    for (auto const& bin : package.bins) {
      std::vector< std::string > deps = {};
      for (auto const& link : bin.links) {
        if (rules.find(libout(link)) != rules.end())
          deps.push_back(libout(link));
      }

      if (deps.size() > 0)
        rules[exeout(bin.name)] += " | " + boost::algorithm::join(deps, " ");
    }
    for (auto const& lib : package.libs) {
      std::vector< std::string > deps = {};
      for (auto const& link : lib.links) {
        if (rules.find(libout(link)) != rules.end())
          deps.push_back(libout(link));
      }

      if (deps.size() > 0)
        rules[libout(lib.name)] += " | " + boost::algorithm::join(deps, " ");
    }

    for (auto const& bin : package.bins) {
      addlinks(exeout(bin.name), bin.links);
    }
    for (auto const& lib : package.libs) {
      addlinks(libout(lib.name), lib.links);
    }
  }

  void build(meka::package const& package) {
    std::clog << "Computing dependencies..." << std::endl;

    std::unordered_map< std::string, std::string > rules;
    meka::genrules(package, rules);

    std::ofstream ninja { (package.path / "build.ninja").string() };
    ninja << "include meka.ninja\n";
    ninja << std::endl;

    for (auto const& pair : rules) {
      ninja << pair.second << std::endl;
    }

    // std::clog << "Building package: " << package.name << ", version: " << package.version << " from: " << package.path << "..." << std::endl;
//    std::system("ninja -d explain -v");
  } // build

}
