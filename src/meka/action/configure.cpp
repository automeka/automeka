/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#include "meka/action/configure.hpp"

#include "meka/path.hpp"
#include "meka/package.hpp"

// #include "corefungi.hpp"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include <unordered_map>
#include <fstream>
#include <sstream>

namespace meka {
  // namespace cfg = ::corefungi;
  namespace bfs = ::boost::filesystem;

  static void genrules(meka::package_type const& package, std::unordered_map< std::string, std::string >& rules) {
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
      rules.emplace(objname, (bfs::extension(source) == ".c" ? "cc  " + source : "cxx " + source) + "\n  incdirs = " + incdirs);
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

    std::transform(std::begin(package.modules), std::end(package.modules), std::back_inserter(idirs), [] (meka::package_type const & module) { return "-I" + (module.path / "include").string(); });
    std::string const& incdirs = boost::algorithm::join(idirs, " ");

    for (meka::package_type const& module : package.modules) {
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

  void configure(meka::package_type const& package) {
    if (!bfs::exists("build"))
      bfs::create_directory("build");

    std::unordered_map< std::string, std::string > rules;
    meka::genrules(package, rules);

    std::ofstream compile_stream {(bfs::current_path() / "build/build.ninja").string()};
    std::ofstream install_stream {(bfs::current_path() / "build/install.ninja").string()};
    std::ofstream package_stream {(bfs::current_path() / "build/package.ninja").string()};

    for(auto stream : {&compile_stream, &install_stream, &package_stream}) {
      *stream << "include meka.ninja\n";
      *stream << "package = " << package.name << "_" << package.version << "\n";
    }

    install_stream << "prefix = /usr/local\n\n"; // << bfs::absolute(cfg::get("meka.prefix")).string() << "\n";
    package_stream << "prefix = ${builddir}/package/${package}\n\n"; // << bfs::absolute(cfg::get("meka.prefix")).string() << "\n";
    package_stream << "sprefix = ${builddir}/package/sources/${package}\n\n"; // << bfs::absolute(cfg::get("meka.prefix")).string() << "\n";


    std::stringstream sources_stream;
    std::string const objprefix {"${builddir}/obj/"};
    for (auto const& pair : rules) {
      compile_stream << "build " << pair.first << ": " << pair.second << "\n";

      if (pair.first.substr(0, objprefix.size()) != objprefix)
        continue;

      auto const from = pair.second.find(' ') + 1;
      auto const& source = std::string{pair.second, from, pair.second.find('\n') - from};
      package_stream << "build ${sprefix}/" << source << ": insfil " << source << "\n";
      sources_stream << "$\n      ${sprefix}/" << source << " ";
    }

    package_stream << "build ${prefix}_sources.tar.xz" << ": packg " << sources_stream.str() << "\n";
    package_stream << "  source = ${package}\n";
    package_stream << "  folder = ${sprefix}/..\n";


    std::stringstream target_stream;
    for (auto const& pair : rules) {
      if (pair.first.substr(0, objprefix.size()) == objprefix)
        continue;

      if (pair.first == "${builddir}/bin/../meka${exeext}")
        continue;

      auto const& target = boost::replace_first_copy(pair.first, "${builddir}", "${prefix}");
      for(auto stream : {&install_stream, &package_stream}) {
        if (pair.first.find("${libext}") == pair.first.size() - 15)
          *stream << "build " << target << ": inslib " << pair.first << "\n";
        else if (pair.first.find("${exeext}") == pair.first.size() - 9)
          *stream << "build " << target << ": insexe " << pair.first << "\n";
        else
          *stream << "build " << target << ": insfil " << pair.first << "\n";
      }

      target_stream << "$\n      " << target << " ";
    }

    package_stream << "build ${prefix}.tar.xz" << ": packg " << target_stream.str() << "\n";
    package_stream << "  source = ${package}\n";
    package_stream << "  folder = ${prefix}/..\n";
  }

}
