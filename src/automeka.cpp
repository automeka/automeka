#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/IRObjectFile.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"

#include <boost/filesystem.hpp>
#include <boost/range/algorithm/mismatch.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/uuid/sha1.hpp>

#include <git2.h>

#include <unordered_set>
#include <unordered_map>
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

cxx = clang++-3.6
lnk = llvm-link-3.6

cflags   = -std=c11 -fpic -g -O3 -fmodules -fautolink
cxxflags = -std=c++1y -fpic -g -O3 -fmodules -fautolink
ldflags  = -O3 -Wl,-O3 -Wl,--gc-sections -L$builddir/lib -Wl,-rpath,$builddir/lib

rule cxx
  command = $cxx -MMD -MT $out -MF $out.d $cxxflags $incdirs -x c++ -c -emit-llvm -o $out $in
  description = ${cylw}CXX${crst} ${cgrn}$out${crst}
  depfile = $out.d
  deps = gcc

rule cc
  command = $cxx -MMD -MT $out -MF $out.d $cflags $incdirs -x c -c -emit-llvm -o $out $in
  description = ${cylw}CC${crst}  ${cgrn}$out${crst}
  depfile = $out.d
  deps = gcc

rule lnk
  command = $lnk -o $out $in
  description = ${cylw}LNK${crst} ${cblu}$out${crst}

rule lib
  command = $cxx -fPIC $ldflags -shared -x ir -o $out $in -Wl,--start-group $libs -Wl,--end-group
  description = ${cylw}LIB${crst} ${cblu}$out${crst}

rule exe
  command = $cxx -fPIC $ldflags -x ir -o $out $in -Wl,--start-group $libs -Wl,--end-group
  description = ${cylw}EXE${crst} ${cblu}$out${crst}

)";

  namespace fs {
    using namespace ::boost::filesystem;

    auto const filename = [](fs::path path) { return std::move(path).filename().generic_string(); };
    auto const stem = [](fs::path path) { return std::move(path).stem().generic_string(); };

    auto const relative = [](fs::path from, fs::path to) {
      auto const pair = boost::mismatch(from, to);

      if (pair.first == std::begin(from))
        return std::move(to);

      auto rel = fs::path {};
      std::for_each(pair.first, from.end(), [&](fs::path const& i) { rel /= ".."; });
      std::for_each(pair.second, to.end(), [&](fs::path const& i) { rel /= i; });

      return std::move(rel);
    };

    auto const is_file = [](fs::path path) {
      switch (fs::status(path).type()) {
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

    auto const is_directory = [](fs::path path) {
      switch (fs::status(path).type()) {
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

  namespace symbol {
    auto const main = "main";

    auto const read = [](fs::path const& path, bool defined) {
      auto&       context         = llvm::getGlobalContext();
      auto        buffer_or_error = llvm::MemoryBuffer::getFile(path.generic_string());
      auto const& buffer          = *buffer_or_error.get();
      auto        binary_or_error = llvm::object::createBinary(buffer.getMemBufferRef(), &context);
      auto const& binary          = *binary_or_error.get();

      auto const from_object = [defined](auto const& object) {
        auto symbols = std::unordered_set<std::string> {};

        for (auto& sym : object.symbols()) {
          llvm::StringRef name;
          if (sym.getName(name))
            continue;

          uint32_t flags = sym.getFlags();
          if (!!(flags & llvm::object::SymbolRef::SF_Undefined) == defined)
            continue;

          llvm::object::SymbolRef::Type type;
          if (sym.getType(type))
            continue;

          if (defined && type != llvm::object::SymbolRef::ST_Data && type != llvm::object::SymbolRef::ST_Function)
            continue;

          if (name.empty() || name[0] == '.')
            continue;

          symbols.insert(name);
        }

        return std::move(symbols);
      };

      auto const from_archive = [&](auto const& archive) {
        auto symbols = std::unordered_set<std::string> {};

        for (auto it = archive.child_begin(), end = archive.child_end(); it != end; ++it) {
          auto        binary_or_error = it->getAsBinary(&context);
          auto const& binary          = *binary_or_error.get();

          if (auto* object = llvm::dyn_cast<llvm::object::ObjectFile>(&binary)) {
            auto objsyms = from_object(*object);
            std::move(std::begin(objsyms), std::end(objsyms), std::inserter(symbols, symbols.begin()));
          }
        }

        return std::move(symbols);
      };

      if (auto* archive = llvm::dyn_cast<llvm::object::Archive>(&binary))
        return from_archive(*archive);
      else if (auto* object = llvm::dyn_cast<llvm::object::ObjectFile>(&binary))
        return from_object(*object);
      else
        return std::unordered_set<std::string> {};
    };

    auto const find = [](fs::path const& path, std::string const& name) {
      auto&       context         = llvm::getGlobalContext();
      auto        buffer_or_error = llvm::MemoryBuffer::getFile(path.generic_string());
      auto const& buffer          = *buffer_or_error.get();
      auto        binary_or_error = llvm::object::createBinary(buffer.getMemBufferRef(), &context);
      auto const& binary          = *binary_or_error.get();

      if (auto* object = llvm::dyn_cast<llvm::object::IRObjectFile>(&binary))
        return object->getModule().getValueSymbolTable().lookup(name) == nullptr;
      else
        return false;
    };

    auto const libraries = [](fs::path const& path) {
      auto flags = std::vector<std::string> {};

      auto&       context         = llvm::getGlobalContext();
      auto        buffer_or_error = llvm::MemoryBuffer::getFile(path.generic_string());
      auto const& buffer          = *buffer_or_error.get();
      auto        binary_or_error = llvm::object::createBinary(buffer.getMemBufferRef(), &context);
      auto const& binary          = *binary_or_error.get();

      if (auto* object = llvm::dyn_cast<llvm::object::IRObjectFile>(&binary)) {
        if (auto* options = llvm::cast<llvm::MDNode>(object->getModule().getModuleFlag("Linker Options"))) {
          for (auto& operand : options->operands()) {
            for (auto& o : llvm::cast<llvm::MDNode>(operand)->operands()) {
              flags.push_back(llvm::cast<llvm::MDString>(o)->getString());
            }
          }
        }
      }

      return flags;
    };

    auto const defined   = [](fs::path const& path) { return symbol::read(path, true); };
    auto const undefined = [](fs::path const& path) { return symbol::read(path, false); };
  }

  namespace system {
    auto const root = "/usr/lib/x86_64-linux-gnu";

    auto const libraries = []() {
      auto libraries = std::vector<fs::path> {};

      for (auto it = fs::directory_iterator {system::root}, end = fs::directory_iterator {}; it != end; ++it) {
        if (!fs::is_file(*it))
          continue;

        auto const path = *it;
        if (fs::extension(path) != extension::arc)
          continue;

        if (fs::extension(fs::stem(path)) == ".dll")
          continue;

        if (!boost::algorithm::starts_with(fs::filename(path), prefix::arc))
          continue;

        libraries.emplace_back(*it);
      }

      return libraries;
    };
  }

  struct project {
    project() = default;
    project(std::string name, fs::path path, std::vector<fs::path> sources, std::vector<fs::path> tests)
      : name(std::move(name)), path(std::move(path)), sources(std::move(sources)), tests(std::move(tests)) {}

    std::string           name;
    fs::path              path;
    std::vector<fs::path> sources;
    std::vector<fs::path> tests;

    std::vector<fs::path> objects  = {};
    std::vector<fs::path> binaries = {};
  };

  auto const find_sources = [](fs::path root) {
    auto sources = std::vector<fs::path> {};

    if (!fs::exists(root / folder::src))
      return std::move(sources);

    for (auto it = fs::recursive_directory_iterator{root / folder::src}, end = fs::recursive_directory_iterator {}; it != end; ++it) {
      auto const path = *it;
      if (!fs::is_file(path))
        continue;

      auto const ext = fs::extension(path);
      if (std::find(std::begin(extension::cpp), std::end(extension::cpp), ext) == std::end(extension::cpp) &&
          std::find(std::begin(extension::c), std::end(extension::c), ext) == std::end(extension::c))
        continue;

      if (boost::algorithm::ends_with(fs::filename(path), suffix::test + ext))
        continue;

      sources.emplace_back(fs::relative(root, *it));
    }

    return std::move(sources);
  };

  auto const find_tests = [](fs::path root) {
    auto tests = std::vector<fs::path> {};

    if (fs::exists(root / folder::src)) {
      for (auto it = fs::recursive_directory_iterator{root / folder::src}, end = fs::recursive_directory_iterator {}; it != end; ++it) {
        auto const path = *it;
        if (!fs::is_file(path))
          continue;

        auto const ext = fs::extension(path);
        if (std::find(std::begin(extension::cpp), std::end(extension::cpp), ext) == std::end(extension::cpp))
          continue;

        if (!boost::algorithm::ends_with(fs::filename(path), suffix::test + ext))
          continue;

        tests.emplace_back(fs::relative(root, *it));
      }
    }

    if (!fs::exists(root / folder::test))
      return std::move(tests);

    for (auto it = fs::recursive_directory_iterator{root / folder::test}, end = fs::recursive_directory_iterator {}; it != end; ++it) {
      auto const path = *it;
      if (!fs::is_file(path))
        continue;

      auto const ext = fs::extension(path);
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
      if (!fs::is_directory(*it))
        continue;

      auto const base = fs::filename(*it);
      if (base == folder::build) {
        it.no_push();
        continue;
      }

      if (base == folder::include && fs::exists(*it / ".." / folder::src))
        continue;

      if (base != folder::src && base != folder::include)
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

      projects.emplace_back(name, relp, find_sources(path), find_tests(path));
    }

    return std::move(projects);
  };

  auto const sha1 = [](fs::path file) {
    git_oid out;
    git_odb_hashfile(&out, file.generic_string().c_str(), GIT_OBJ_BLOB);
    return out;
  };

  extern "C" int main(int, char*[]) {
    auto const root      = fs::current_path();
    auto const ninja     = std::string { std::getenv("NINJA") ? std::getenv("NINJA") : "ninja" };

    auto const builddir  = fs::path(folder::build);
    auto const objdir    = builddir / "obj";
    auto const libdir    = builddir / "lib";
    auto const bindir    = builddir / "bin";


    // auto folders = { "/usr/lib/x86_64-linux-gnu", "/usr/include", "lib", "src" };

    // for (auto f : folders) {
    //   for (auto it = fs::directory_iterator{f}, end = fs::directory_iterator {}; it != end; ++it) {
    //     auto const path = *it;
    //     if (!fs::is_file(path))
    //       continue;

    //     if (fs::is_symlink(path))
    //       continue;

    //     std::cerr << path << std::endl;
    //     sha1(path);
    //   }
    // }

    // return 0;


    auto projects = find_projects(root);

    {
      auto const ninjafile = (objdir / "build.ninja").generic_string();
      fs::create_directories(objdir);

      {
        auto && out = std::ofstream { ninjafile };
        out << mekaninja;

        auto incdirs = std::vector<std::string> {};
        std::transform(std::begin(projects), std::end(projects), std::back_inserter(incdirs), [&root](auto project) { return "-I" + (project.path / folder::include).generic_string(); });

        for (auto const& p : projects) {
          for (auto const& s : p.sources) {
            if (std::find(std::begin(extension::c), std::end(extension::c), fs::extension(s)) == std::end(extension::c))
              out << "build " << fs::change_extension(objdir / p.name / s, extension::obj).generic_string() << ": cxx " << (p.path / s).generic_string() << "\n";
            else
              out << "build " << fs::change_extension(objdir / p.name / s, extension::obj).generic_string() << ": cc " << (p.path / s).generic_string() << "\n";
            out << "  incdirs = -I" << (p.path / folder::src).generic_string() << " " << boost::algorithm::join(incdirs, " ") << "\n";
            out << "  module = " << p.name << "\n";
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
      auto const ninjafile = (libdir / "build.ninja").generic_string();
      fs::create_directories(libdir);

      {
        auto && out = std::ofstream { ninjafile };
        out << mekaninja;

        for (auto& p : projects) {
          for (auto const& s : p.sources) {
            auto const object  = fs::change_extension(objdir / p.name / s, extension::obj);

            if (symbol::find(object, symbol::main))
              p.objects.emplace_back(std::move(object));
            else
              p.binaries.emplace_back(std::move(object));
          }

          auto objects = std::vector<std::string> {};
          std::transform(std::begin(p.objects), std::end(p.objects), std::back_inserter(objects), [](auto path) { return path.generic_string(); });

          if (objects.empty())
            continue;

          out << "build " << (objdir / (prefix::arc + p.name)).generic_string() << extension::obj << ": lnk $\n";
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
      auto const ninjafile = (bindir / "build.ninja").generic_string();
      fs::create_directories(bindir);

      {
        auto && out = std::ofstream { ninjafile };
        out << mekaninja;

        auto libraries = std::unordered_set<std::string> {};
        for (auto& p : projects) {
          auto object = (objdir / (prefix::arc + p.name)).generic_string() + extension::obj;
          if (!fs::exists(object))
            continue;

          libraries.insert("-l" + p.name);
          out << "build " << (libdir / (prefix::lib + p.name)).generic_string() << extension::lib << ": lib " << object << "\n";
        }

        for (auto const& p : projects) {
          for (auto const& o : p.binaries) {
            auto linklibs = symbol::libraries(o);

            out << "build " << (bindir / p.name / (fs::basename(o) + extension::exe)).generic_string() << ": exe " << o.generic_string() << " |";
            for (auto l : linklibs) {
              if (libraries.find(l) == libraries.end())
                continue;

              out << " $\n";
              out << "  " << (libdir / (prefix::lib + l.substr(2))).generic_string() << extension::lib;
            }
            out << "\n";
            out << "  libs = " << boost::algorithm::join(linklibs, " ") << "\n";
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
