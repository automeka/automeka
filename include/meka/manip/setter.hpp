/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef __MEKA_MANIP_SETTER_HPP__
#define __MEKA_MANIP_SETTER_HPP__

namespace meka {

  template< typename T, template< typename S > class D >
  struct field_setter {
    constexpr field_setter() {}

    template< typename ... Ts >
    explicit constexpr field_setter(Ts&& ... ts) :
      data { std::forward< Ts >(ts) ... }
    {}

    template< typename U >
    constexpr field_setter operator=(U&& v) const { return field_setter { std::forward< U >(v) }; }

    constexpr field_setter operator=(T&& v) const { return field_setter { std::move(v) }; }

    template< typename S >
    constexpr operator std::function< void(S&) >() const { return [ = ](S & subject)->void { subject.*D< S >::pointer = this->data; }; }

    T const data;
  };

  template< typename S > struct path_field {
    static constexpr bfs::path S::* pointer = &S::path;
  };
  static field_setter< bfs::path, path_field > const path;

  template< typename S > struct name_field {
    static constexpr std::string S::* pointer = &S::name;
  };
  static field_setter< std::string, name_field > const name;

  template< typename S > struct version_field {
    static constexpr std::string S::* pointer = &S::version;
  };
  static field_setter< std::string, version_field > const version;

  template< typename S > struct sources_field {
    static constexpr std::vector< std::string > S::* pointer = &S::sources;
  };
  static field_setter< std::vector< std::string >, sources_field > const sources;

  template< typename S > struct links_field {
    static constexpr std::vector< std::string > S::* pointer = &S::links;
  };
  static field_setter< std::vector< std::string >, links_field > const links;

  template< typename S > struct linkage_field {
    static constexpr std::string S::* pointer = &S::linkage;
  };
  static field_setter< std::string, linkage_field > const linkage;

  template< typename S > struct modules_field {
    static constexpr std::vector< meka::module_type > S::* pointer = &S::modules;
  };
  static field_setter< std::vector< meka::module_type >, modules_field > const modules;

}

#endif // ifndef __MEKA_MANIP_SETTER_HPP__
