/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#include "meka/action/configure.hpp"

#include "meka/path.hpp"
#include "meka/package.hpp"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include <unordered_map>
#include <fstream>

namespace meka {
  namespace bfs = ::boost::filesystem;

  static void genrules(meka::package const& package, std::unordered_map< std::string, std::string >& rules) {
    auto const& objout = [] (std::string const & name) {
      return "${builddir}/obj/" + name + "${objext}";
    };
    auto const& libout = [] (std::string const & name, std::string const & version) {
      return "${builddir}/lib/${libpfx}" + name + "${libext}" + (version.empty() ? "" : "." + version);
    };
    auto const& exeout = [] (std::string const & name) {
      return "${builddir}/bin/" + name + "${exeext}";
    };
    auto const& incout = [] (std::string const & name) {
      return "${builddir}/" + name;
    };

    auto const& addbin = [&](std::string const & binname, std::string const & type) {
      std::string const& binout = type == "lib" ? libout(binname, package.version) : exeout(binname);
      rules.emplace(binout, type);
    };
    auto const& addobj = [&](std::string const & binname, std::string const & objname, std::string const & source, std::string const & incdirs) {
      rules.emplace(objname, "cxx " + source + "\n  incdirs = " + incdirs);
      rules[binname] += " " + objname;
    };
    auto const& addlinks = [&](std::string const & binname, std::vector< std::string > const & links) {
      rules[binname] += "\n  libs =";
      for (auto const& link : links) {
        rules[binname] += " -l" + link;
      }
    };
    auto const& addlns = [&](std::string const & libname, std::string const & version) {
      std::string const& soversion = version.substr(0, version.find('.'));
      rules.emplace(libout(libname, soversion), "ln " + libout(libname, version));
      rules.emplace(libout(libname, ""), "ln " + libout(libname, soversion));
    };
    auto const& addincl = [&](std::string const & target, std::string const & source) {
      rules.emplace(target, "copy " + source);
    };

    std::vector< std::string > idirs = { "-I" + (package.path / "src").string(), "-I" + (package.path / "include").string() };

    std::transform(std::begin(package.modules), std::end(package.modules), std::back_inserter(idirs), [] (meka::package const & module) { return "-I" + (module.path / "include").string(); });
    std::string const& incdirs = boost::algorithm::join(idirs, " ");

    for (meka::package const& module : package.modules) {
      meka::genrules(module, rules);
    }

    for (auto const& bin : package.bins) {
      addbin(bin.name, "exe");
    }
    for (auto const& lib : package.libs) {
      addbin(lib.name, "lib");
      addlns(lib.name, package.version);
    }

    for (bfs::recursive_directory_iterator fit { bfs::current_path() / package.path }, end; fit != end; ++fit) {
      switch (fit.status().type()) {
        default:
        case bfs::status_error:
        case bfs::file_not_found:
        case bfs::directory_file:
        case bfs::type_unknown:
          continue;

        case bfs::regular_file:
        case bfs::symlink_file:
        case bfs::block_file:
        case bfs::character_file:
        case bfs::fifo_file:
        case bfs::socket_file:
          break;
      }

      bfs::path const   path  = *fit;
      std::string const spath = meka::relative_path(bfs::current_path() / package.path, path).string();
      std::string const opath = meka::relative_path(bfs::current_path(), path.parent_path() / path.stem()).string();

      if (boost::regex_match(std::begin(spath), std::end(spath), boost::regex { "include/.*" }))
        addincl(incout(spath), (package.path / spath).string());

      for (auto const& bin : package.bins) {
        for (auto const& src : bin.sources) {
          if (boost::regex_match(std::begin(spath), std::end(spath), boost::regex { src }))
            addobj(exeout(bin.name), objout(opath), (package.path / spath).string(), incdirs);
        }
      }
      for (auto const& lib : package.libs) {
        for (auto const& src : lib.sources) {
          if (boost::regex_match(std::begin(spath), std::end(spath), boost::regex { src }))
            addobj(libout(lib.name, package.version), objout(opath), (package.path / spath).string(), incdirs);
        }
      }
    }

    for (auto const& bin : package.bins) {
      std::vector< std::string > deps = {};
      for (auto const& link : bin.links) {
        if (rules.find(libout(link, "")) != rules.end())
          deps.push_back(libout(link, ""));
      }

      if (deps.size() > 0)
        rules[exeout(bin.name)] += " | " + boost::algorithm::join(deps, " ");
    }
    for (auto const& lib : package.libs) {
      std::vector< std::string > deps = {};
      for (auto const& link : lib.links) {
        if (rules.find(libout(link, "")) != rules.end())
          deps.push_back(libout(link, ""));
      }

      if (deps.size() > 0)
        rules[libout(lib.name, package.version)] += " | " + boost::algorithm::join(deps, " ");
    }

    for (auto const& bin : package.bins) {
      addlinks(exeout(bin.name), bin.links);
    }
    for (auto const& lib : package.libs) {
      addlinks(libout(lib.name, package.version), lib.links);
    }
  } // genrules

  void configure(meka::package const& package) {
    if (!bfs::exists("build"))
      bfs::create_directory("build");

    std::unordered_map< std::string, std::string > rules;
    meka::genrules(package, rules);

    std::ofstream build {(bfs::current_path() / "build/build.ninja").string()};

    build << "include meka.ninja\n";
    build << std::endl;

    for (auto const& pair : rules) {
      build << "build " << pair.first << ": " << pair.second << std::endl;
    }

    std::ofstream install {(bfs::current_path() / "build/install.ninja").string()};

    install << "include meka.ninja\n";
    install << "prefix = /usr/local\n";
    install << std::endl;

    std::string const objprefix {"${builddir}/obj/"};
    for (auto const& pair : rules) {
      if (pair.first.substr(0, objprefix.size()) == objprefix)
        continue;

      if (pair.first == "${builddir}/bin/../meka${exeext}")
        continue;

      if (pair.first.find("${libext}") == pair.first.size() - 15)
        install << "build " << boost::replace_first_copy(pair.first, "${builddir}", "${prefix}") << ": inslib " << pair.first << std::endl;
      else if (pair.first.find("${exeext}") == pair.first.size() - 9)
        install << "build " << boost::replace_first_copy(pair.first, "${builddir}", "${prefix}") << ": insexe " << pair.first << std::endl;
      else
        install << "build " << boost::replace_first_copy(pair.first, "${builddir}", "${prefix}") << ": insfil " << pair.first << std::endl;
    }
  }

}
