#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/IRObjectFile.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include <experimental/filesystem>

namespace meka {
  namespace algo {
    auto const join = [](auto&& list, auto delim) {
      if (list.size() == 0)
        return std::string{};

      if (list.size() == 1)
        return list[0];

      std::ostringstream joined;
      std::copy(std::begin(list), std::end(list) - 1,
                std::ostream_iterator< std::string >(joined, std::move(delim)));
      joined << *std::rbegin(list);

      return joined.str();
    };

    auto const ends_with = [](auto string, auto end) {
      if (string.size() < end.size())
        return false;
      return std::equal(std::rbegin(end), std::rend(end), std::rbegin(string));
    };
  }

  namespace folder {
    auto const build   = "build";
    auto const src     = "src";
    auto const include = "include";
    auto const test    = "test";
  }

  namespace extension {
    auto const cpp = {".cpp", ".cc", ".C", ".c++", ".cxx"};
    auto const hpp = {".hpp", ".hh", ".H", ".h++", ".hxx"};
    auto const c   = {".c"};
    auto const h   = {".h"};
    auto const obj = ".o";
    auto const arc = ".a";
    auto const lib = ".so";
    auto const exe = "";
  }

  namespace suffix {
    auto const test = "_test";
  }

  namespace prefix {
    auto const arc = "lib";
    auto const lib = "lib";
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

cxx = clang++-3.9
lnk = llvm-link-3.9

ccflags = -O3 -fPIC -fmodules -fautolink -ffunction-sections -fdata-sections
defines = -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
ldflags = -Wl,-O3 -Wl,-s -Wl,--gc-sections -L$builddir/lib -Wl,-rpath,\$$ORIGIN/../../lib

rule cxx
  command = $cxx -x c++ -std=c++1z -o $out $in -c -emit-llvm -MMD -MT $out -MF $out.d $ccflags $defines $incdirs -fmodule-name=$module
  description = ${cylw}CXX${crst} ${cgrn}$out${crst}
  depfile = $out.d
  deps = gcc

rule cc
  command = $cxx -x c   -std=c11   -o $out $in -c -emit-llvm -MMD -MT $out -MF $out.d $ccflags $defines $incdirs -fmodule-name=$module
  description = ${cylw}CC${crst}  ${cgrn}$out${crst}
  depfile = $out.d
  deps = gcc

rule lnk
  command = $lnk -o $out $in
  description = ${cylw}LNK${crst} ${cblu}$out${crst}

rule lib
  command = $cxx -x ir -o $out $in -shared $ccflags $ldflags -fmodule-name=$module -Wl,--start-group $libs -Wl,--end-group
  description = ${cylw}LIB${crst} ${cblu}$out${crst}

rule exe
  command = $cxx -x ir -o $out $in         $ccflags $ldflags -fmodule-name=$module -Wl,--start-group $libs -Wl,--end-group
  description = ${cylw}EXE${crst} ${cblu}$out${crst}

)";

  namespace fs {
    using namespace ::std::experimental::filesystem;

    auto const filename = [](fs::path path) { return std::move(path).filename().generic_string(); };
    auto const stem     = [](fs::path path) { return std::move(path).stem().generic_string(); };
    auto const extension
      = [](fs::path path) { return std::move(path).extension().generic_string(); };

    auto const replace_extension = [](fs::path path, fs::path ext) {
      return std::move(path).replace_extension(std::move(ext));
    };

    auto const relative = [](fs::path from, fs::path to) {
      auto const pair = std::distance(std::begin(to), std::end(to))
                            < std::distance(std::begin(from), std::end(from))
                          ? std::mismatch(std::begin(to), std::end(to), std::begin(from))
                          : std::mismatch(std::begin(from), std::end(from), std::begin(to));

      if (pair.first == std::begin(from))
        return std::move(to);

      auto rel = fs::path{};
      std::for_each(pair.first, from.end(), [&](fs::path const& i) { rel /= ".."; });
      std::for_each(pair.second, to.end(), [&](fs::path const& i) { rel /= i; });

      return std::move(rel);
    };
  }

  namespace module {
    auto const main = "main";

    auto const contains = [](fs::path const& path, std::string const& name) {
      auto&       context         = llvm::getGlobalContext();
      auto        buffer_or_error = llvm::MemoryBuffer::getFile(path.generic_string());
      auto const& buffer          = *buffer_or_error.get();
      auto        binary_or_error = llvm::object::createBinary(buffer.getMemBufferRef(), &context);
      auto const& binary          = *binary_or_error.get();

      if (auto* object = llvm::dyn_cast< llvm::object::IRObjectFile >(&binary))
        return object->getModule().getValueSymbolTable().lookup(name) == nullptr;
      else
        return false;
    };

    auto const libraries = [](fs::path const& path) {
      auto flags = std::vector< std::string >{};

      auto&       context         = llvm::getGlobalContext();
      auto        buffer_or_error = llvm::MemoryBuffer::getFile(path.generic_string());
      auto const& buffer          = *buffer_or_error.get();
      auto        binary_or_error = llvm::object::createBinary(buffer.getMemBufferRef(), &context);
      auto&       binary          = *binary_or_error.get();

      if (auto* object = llvm::dyn_cast< llvm::object::IRObjectFile >(&binary)) {
        object->getModule().materializeMetadata();

        if (auto* options
            = llvm::cast< llvm::MDNode >(object->getModule().getModuleFlag("Linker Options"))) {
          for (auto& operand : options->operands()) {
            for (auto& o : llvm::cast< llvm::MDNode >(operand)->operands()) {
              flags.push_back(llvm::cast< llvm::MDString >(o)->getString());
            }
          }
        }
      }

      return flags;
    };
  }

  struct project {
    project() = default;
    project(std::string name, fs::path path, std::vector< fs::path > sources)
      : name(std::move(name)), path(std::move(path)), sources(std::move(sources)) {}

    std::string             name;
    fs::path                path;
    std::vector< fs::path > sources;

    std::vector< fs::path > objects  = {};
    std::vector< fs::path > binaries = {};
  };

  auto const find_sources = [](fs::path root) {
    auto sources = std::vector< fs::path >{};

    if (!fs::exists(root / folder::src))
      return std::move(sources);

    for (auto&& path : fs::recursive_directory_iterator{root / folder::src}) {
      if (!fs::is_regular_file(path) && !fs::is_symlink(path))
        continue;

      auto const ext = fs::extension(path);
      if (std::find(std::begin(extension::cpp), std::end(extension::cpp), ext)
            == std::end(extension::cpp)
          && std::find(std::begin(extension::c), std::end(extension::c), ext)
               == std::end(extension::c))
        continue;

      if (algo::ends_with(fs::filename(path), suffix::test + ext))
        continue;

      sources.emplace_back(fs::relative(root, path));
    }

    return std::move(sources);
  };

  auto const find_tests = [](fs::path root) {
    auto tests = std::vector< fs::path >{};

    if (fs::exists(root / folder::src)) {
      for (auto&& path : fs::recursive_directory_iterator{root / folder::src}) {
        if (!fs::is_regular_file(path) && !fs::is_symlink(path))
          continue;

        auto const ext = fs::extension(path);
        if (std::find(std::begin(extension::cpp), std::end(extension::cpp), ext)
              == std::end(extension::cpp)
            && std::find(std::begin(extension::c), std::end(extension::c), ext)
                 == std::end(extension::c))
          continue;

        if (!algo::ends_with(fs::filename(path), suffix::test + ext))
          continue;

        tests.emplace_back(fs::relative(root, path));
      }
    }

    if (!fs::exists(root / folder::test))
      return std::move(tests);

    for (auto&& path : fs::recursive_directory_iterator{root / folder::test}) {
      if (!fs::is_regular_file(path) && !fs::is_symlink(path))
        continue;

      auto const ext = fs::extension(path);
      if (std::find(std::begin(extension::cpp), std::end(extension::cpp), ext)
            == std::end(extension::cpp)
          && std::find(std::begin(extension::c), std::end(extension::c), ext)
               == std::end(extension::c))
        continue;

      tests.emplace_back(fs::relative(root, path));
    }

    return std::move(tests);
  };

  auto const find_projects = [](fs::path root) {
    auto names    = std::unordered_set< std::string >{};
    auto projects = std::vector< project >{};

    for (auto it = fs::recursive_directory_iterator{root}, end = fs::recursive_directory_iterator{};
         it != end; ++it) {
      if (!fs::is_directory(*it))
        continue;

      auto const base = fs::filename(*it);
      if (base == folder::build) {
        it.disable_recursion_pending();
        continue;
      }

      if (base == folder::include && fs::exists(*it / ".." / folder::src))
        continue;

      if (base != folder::src && base != folder::include)
        continue;

      it.disable_recursion_pending();

      auto const path = fs::path(*it).parent_path();
      auto const relp = fs::relative(root, path);
      auto const name = fs::filename(path);
      if (names.find(name) != names.end()) {
        std::cerr << "W: project " << name << " in " << path << " already found, ignoring\n";
        continue;
      }
      names.insert(name);

      projects.emplace_back(name, relp, find_sources(path));

      if (path == root)
        projects.emplace_back(name + "_test", relp, find_tests(path));
    }

    return std::move(projects);
  };

  extern "C" int main(int, char* []) {
    // FIXME: fs::current_path() is currently broken
    auto const root  = fs::path(".");
    auto const ninja = std::string{std::getenv("NINJA") ? std::getenv("NINJA") : "ninja"};

    auto const builddir = fs::path(folder::build);
    auto const objdir   = builddir / "obj";
    auto const libdir   = builddir / "lib";
    auto const bindir   = builddir / "bin";

    auto projects = find_projects(root);

    {
      auto const ninjafile = (objdir / "build.ninja").generic_string();
      fs::create_directories(objdir);

      {
        auto&& out = std::ofstream{ninjafile};
        out << mekaninja;

        auto incdirs = std::vector< std::string >{};
        std::transform(std::begin(projects), std::end(projects), std::back_inserter(incdirs),
                       [&root](auto project) {
                         return "-I" + (project.path / folder::include).generic_string();
                       });

        for (auto const& p : projects) {
          for (auto const& s : p.sources) {
            if (std::find(std::begin(extension::c), std::end(extension::c), fs::extension(s))
                == std::end(extension::c))
              out << "build "
                  << fs::replace_extension(objdir / p.name / s, extension::obj).generic_string()
                  << ": cxx " << (p.path / s).generic_string() << "\n";
            else
              out << "build "
                  << fs::replace_extension(objdir / p.name / s, extension::obj).generic_string()
                  << ": cc " << (p.path / s).generic_string() << "\n";
            out << "  incdirs = -I" << (p.path / folder::src).generic_string() << " "
                << algo::join(incdirs, " ") << "\n";
            out << "  module = " << p.name << "\n";
            out << "\n";
          }
        }
      }

      auto const command = ninja + " -f " + ninjafile + " ";
      auto const result  = std::system(command.c_str());

      if (result)
        return result;
    }

    {
      auto const ninjafile = (libdir / "build.ninja").generic_string();
      fs::create_directories(libdir);

      {
        auto&& out = std::ofstream{ninjafile};
        out << mekaninja;

        for (auto& p : projects) {
          for (auto const& s : p.sources) {
            auto const object = fs::replace_extension(objdir / p.name / s, extension::obj);

            if (module::contains(object, module::main))
              p.objects.emplace_back(std::move(object));
            else
              p.binaries.emplace_back(std::move(object));
          }

          auto objects = std::vector< std::string >{};
          std::transform(std::begin(p.objects), std::end(p.objects), std::back_inserter(objects),
                         [](auto path) { return path.generic_string(); });

          if (objects.empty())
            continue;

          out << "build " << (objdir / (prefix::arc + p.name)).generic_string() << extension::obj
              << ": lnk $\n";
          for (auto const& o : objects)
            out << "  " << o << " $\n";
          out << "\n";
        }
      }

      auto const command = ninja + " -f " + ninjafile + " ";
      auto const result  = std::system(command.c_str());

      if (result)
        return result;
    }

    {
      auto const ninjafile = (bindir / "build.ninja").generic_string();
      fs::create_directories(bindir);

      {
        auto&& out = std::ofstream{ninjafile};
        out << mekaninja;

        auto libraries = std::unordered_set< std::string >{};
        for (auto& p : projects) {
          auto object = (objdir / (prefix::arc + p.name)).generic_string() + extension::obj;
          if (!fs::exists(object))
            continue;

          libraries.insert("-l" + p.name);
          out << "build " << (libdir / (prefix::lib + p.name)).generic_string() << extension::lib
              << ": lib " << object << "\n";
          out << "  module = " << p.name << "\n";
        }

        for (auto const& p : projects) {
          for (auto const& o : p.binaries) {
            auto linklibs = module::libraries(o);

            auto self = "-l" + p.name;
            if (libraries.find(self) != libraries.end())
              linklibs.emplace_back(self);

            out << "build " << (bindir / p.name / (fs::stem(o) + extension::exe)).generic_string()
                << ": exe " << o.generic_string() << " |";
            for (auto l : linklibs) {
              if (libraries.find(l) == libraries.end())
                continue;

              out << " $\n";
              out << "  " << (libdir / (prefix::lib + l.substr(2))).generic_string()
                  << extension::lib;
            }
            out << "\n";
            out << "  libs = " << algo::join(linklibs, " ") << "\n";
            out << "  module = " << p.name << "\n";
            out << "\n";
          }
        }
      }

      auto const command = ninja + " -f " + ninjafile + " ";
      auto const result  = std::system(command.c_str());

      if (result)
        return result;
    }

    return 0;
  }
}
