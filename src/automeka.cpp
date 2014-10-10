#include <boost/filesystem.hpp>
#include <boost/range/algorithm/mismatch.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <unordered_set>
#include <fstream>
#include <iostream>

namespace meka {
  namespace folder {
    auto const build   = "build";
    auto const src     = "src";
    auto const include = "include";
    auto const test    = "test";
  }

  namespace extension {
    auto const cpp = { ".cpp", ".cc", ".C", ".c++", ".cxx" };
    auto const hpp = { ".hpp", ".hh", ".H", ".h++", ".hxx" };
    auto const c   = { ".c" };
    auto const h   = { ".h" };
    auto const obj = ".o";
    auto const arc = ".a";
  }

  namespace suffix {
    auto const test = "_test";
  }

  auto const mekaninja = R"(
ninja_required_version = 1.3
builddir = build

cblk = [30m
cred = [31m
cgrn = [32m
cylw = [33m
cblu = [34m
cprp = [35m
ccyn = [36m
cwht = [37m
crgb = [38m
cdef = [39m
crst = [0m

cbblk = ${cblk}[1m
cbred = ${cred}[1m
cbgrn = ${cgrn}[1m
cbylw = ${cylw}[1m
cbblu = ${cblu}[1m
cbprp = ${cprp}[1m
cbcyn = ${ccyn}[1m
cbwht = ${cwht}[1m
cbrgb = ${crgb}[1m
cbdef = ${cdef}[1m
cbrst = ${crst}[1m

cxx   = clang++-3.6
cc    = clang-3.6
ar    = ar

cflags   = -std=c11 -fpic -g -O0
cxxflags = -std=c++1y -fpic -g -O0
ldflags  = -L$builddir/lib -Wl,-rpath,$builddir/lib -L/usr/local/lib

rule cxx
  command = $cxx -MMD -MT $out -MF $out.d $cxxflags $incdirs -xc++ -c $in -o $out
  description = ${cylw}CXX${crst} ${cgrn}$out${crst}
  depfile = $out.d
  deps = gcc

rule cc
  command = $cc -MMD -MT $out -MF $out.d $cflags $incdirs -xc -c $in -o $out
  description = ${cylw}CXX${crst} ${cgrn}$out${crst}
  depfile = $out.d
  deps = gcc

rule arc
  command = rm -f $out && $ar crs $out $in
  description = ${cylw}AR${crst}  ${cblu}$out${crst}

rule exe
  command = $cxx -o $out $in $ldflags -Wl,--start-group $libs -Wl,--end-group
  description = ${cylw}EXE${crst} ${cblu}$out${crst}

)";

  namespace fs {
    using namespace ::boost::filesystem;

    auto const filename = [](fs::path path) { return std::move(path).filename().string(); };

    auto const relative = [](fs::path from, fs::path to) {
      auto const pair = boost::mismatch(from, to);

      if (pair.first == std::begin(from))
        return std::move(to);

      auto rel = fs::path {};
      std::for_each(pair.first, from.end(), [&](fs::path const& i) { rel /= ".."; });
      std::for_each(pair.second, to.end(), [&](fs::path const& i) { rel /= i; });

      return std::move(rel);
    };

    auto const is_file = [](auto it) {
      switch (it.status().type()) {
        default:
        case fs::status_error:
        case fs::file_not_found:
        case fs::directory_file:
        case fs::type_unknown:
          return false;

        case fs::regular_file:
        case fs::symlink_file:
        case fs::block_file:
        case fs::character_file:
        case fs::fifo_file:
        case fs::socket_file:
          return true;
      }
    };

    auto const is_directory = [](auto it) {
      switch (it.status().type()) {
        default:
        case fs::status_error:
        case fs::file_not_found:
        case fs::type_unknown:
        case fs::regular_file:
        case fs::symlink_file:
        case fs::block_file:
        case fs::character_file:
        case fs::fifo_file:
        case fs::socket_file:
          return false;

        case fs::directory_file:
          return true;
      }
    };
  }

  struct project {
    std::string           name;
    fs::path              path;
    std::vector<fs::path> sources;
    std::vector<fs::path> tests;

    std::vector<fs::path> objects  = {};
    std::vector<fs::path> binaries = {};
  };

  auto const find_sources = [](fs::path root) {
    auto sources = std::vector<fs::path> {};

    for (auto it = fs::recursive_directory_iterator{root / folder::src}, end = fs::recursive_directory_iterator {}; it != end; ++it) {
      if (!fs::is_file(it))
        continue;

      auto const path = *it;
      auto const ext  = fs::extension(path);
      if (std::find(std::begin(extension::cpp), std::end(extension::cpp), ext) == std::end(extension::cpp))
        continue;

      if (boost::algorithm::ends_with(fs::filename(path), suffix::test + ext))
        continue;

      sources.emplace_back(fs::relative(root, *it));
    }

    return std::move(sources);
  };

  auto const find_tests = [](fs::path root) {
    auto tests = std::vector<fs::path> {};

    for (auto it = fs::recursive_directory_iterator{root / folder::src}, end = fs::recursive_directory_iterator {}; it != end; ++it) {
      if (!fs::is_file(it))
        continue;

      auto const path = *it;
      auto const ext  = fs::extension(path);
      if (std::find(std::begin(extension::cpp), std::end(extension::cpp), ext) == std::end(extension::cpp))
        continue;

      if (!boost::algorithm::ends_with(fs::filename(path), suffix::test + ext))
        continue;

      tests.emplace_back(fs::relative(root, *it));
    }

    if (!fs::exists(root / folder::test))
      return std::move(tests);

    for (auto it = fs::recursive_directory_iterator{root / folder::test}, end = fs::recursive_directory_iterator {}; it != end; ++it) {
      if (!fs::is_file(it))
        continue;

      auto const path = *it;
      auto const ext  = fs::extension(path);
      if (std::find(std::begin(extension::cpp), std::end(extension::cpp), ext) == std::end(extension::cpp))
        continue;

      tests.emplace_back(fs::relative(root, *it));
    }

    return std::move(tests);
  };

  auto const find_projects = [](fs::path root) {
    auto names    = std::unordered_set<std::string> {};
    auto projects = std::vector<project> {};

    for (auto it = fs::recursive_directory_iterator{root}, end = fs::recursive_directory_iterator {}; it != end; ++it) {
      if (!fs::is_directory(it))
        continue;

      auto const base = fs::filename(*it);
      if (base == folder::build) {
        it.no_push();
        continue;
      }

      if (base != folder::src)
        continue;

      it.no_push();

      auto const path = fs::path(*it).parent_path();
      auto const relp = fs::relative(root, path);
      auto const name = fs::filename(path);
      if (names.find(name) != names.end()) {
        std::cerr << "W: project " << name << " in " << path << " already found, ignoring\n";
        continue;
      }
      names.insert(name);

      projects.push_back({ name, relp, find_sources(path), find_tests(path) });
    }

    return std::move(projects);
  };

  extern "C" int main(int, char*[]) {
    auto const root      = fs::current_path();
    auto const ninja     = std::string { std::getenv("NINJA") ? std::getenv("NINJA") : "ninja" };

    auto const builddir  = fs::path(folder::build);
    auto const objdir    = builddir / "obj";
    auto const libdir    = builddir / "lib";
    auto const bindir    = builddir / "bin";

    auto projects = find_projects(root);

    {
      auto const ninjafile = (objdir / "build.ninja").string();
      fs::create_directories(objdir);

      {
        auto && out = std::ofstream { ninjafile };
        out << mekaninja;

        auto incdirs = std::vector<std::string> {};
        std::transform(std::begin(projects), std::end(projects), std::back_inserter(incdirs), [&root](auto project) { return "-I" + (project.path / folder::include).string(); });

        for (auto const& p : projects) {
          for (auto const& s : p.sources) {
            out << "build " << fs::change_extension(objdir / p.name / s, extension::obj).string() << ": cxx " << (p.path / s).string() << "\n";
            out << "  incdirs = -I" << (p.path / folder::src).string() << " " << boost::algorithm::join(incdirs, " ") << "\n";
            out << "\n";
          }
        }
      }

      auto const command = ninja + " -f " + ninjafile + " -k0";
      auto const result  = std::system(command.c_str());

      if (result)
        return result;
    }

    {
      auto const ninjafile = (libdir / "build.ninja").string();
      fs::create_directories(libdir);

      {
        auto && out = std::ofstream { ninjafile };
        out << mekaninja;

        for (auto& p : projects) {
          for (auto const& s : p.sources) {
            auto const object  = fs::change_extension(objdir / p.name / s, extension::obj);
            auto const command = "nm " + object.string() + " --defined-only | grep --quiet ' T main'";
            auto const result  = std::system(command.c_str());

            if (result)
              p.objects.emplace_back(std::move(object));
            else
              p.binaries.emplace_back(std::move(object));
          }

          auto objects = std::vector<std::string> {};
          std::transform(std::begin(p.objects), std::end(p.objects), std::back_inserter(objects), [](auto path) { return path.string(); });

          out << "build " << (libdir / p.name).string() << extension::arc << ": arc $\n";
          for (auto const& o : objects)
            out << "  " << o << " $\n";
          out << "\n";
        }
      }

      auto const command = ninja + " -f " + ninjafile + " -k0";
      auto const result  = std::system(command.c_str());

      if (result)
        return result;
    }

    {
      auto const ninjafile = (bindir / "build.ninja").string();
      fs::create_directories(bindir);

      {
        auto && out = std::ofstream { ninjafile };
        out << mekaninja;

        auto archives = std::vector<std::string> {};
        std::transform(std::begin(projects), std::end(projects), std::back_inserter(archives), [libdir](auto project) { return (libdir / project.name).string() + extension::arc; });

        for (auto const& p : projects) {
          for (auto const& o : p.binaries) {
            out << "build " << (bindir / p.name / fs::basename(o)).string() << ": exe " << o.string() << " | " << boost::algorithm::join(archives, " $\n    ") << "\n";
            out << "  libs = " << boost::algorithm::join(archives, " ") << "\n";
            out << "\n";
          }
        }
      }

      auto const command = ninja + " -f " + ninjafile + " -k0";
      auto const result  = std::system(command.c_str());

      if (result)
        return result;
    }

    return 0;
  }

}
