/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef __MEKA_MANIP_SETTER_HPP__
#define __MEKA_MANIP_SETTER_HPP__

#include "meka/manip/manipulable.hpp"

namespace meka {

  template< typename T, template< typename Subject > class Field >
  struct field_setter {
    template< typename ... Ts >
    constexpr explicit field_setter(Ts&& ... ts) :
      data { std::forward< Ts >(ts) ... }
    {}

    constexpr field_setter operator=(T&& v) const
    { return field_setter { std::move(v) }; }

    template< typename Subject >
    constexpr operator std::function< void(Subject&) >() const
    { return [ = ](Subject & subject) { (subject.*Field< Subject >::pointer) = this->data; }; }

    T const data;
  };

  template< typename Subject > struct path_field {
    static constexpr bfs::path Subject::* const pointer = &Subject::path;
  };
  static field_setter< bfs::path, path_field > const path;

  template< typename Subject > struct name_field {
    static constexpr std::string Subject::* const pointer = &Subject::name;
  };
  static field_setter< std::string, name_field > const name;

  template< typename Subject > struct version_field {
    static constexpr std::string Subject::* const pointer = &Subject::version;
  };
  static field_setter< std::string, version_field > const version;

  template< typename Subject > struct sources_field {
    static constexpr std::vector< std::string > Subject::* const pointer = &Subject::sources;
  };
  static field_setter< std::vector< std::string >, sources_field > const sources;

  template< typename Subject > struct links_field {
    static constexpr std::vector< std::string > Subject::* const pointer = &Subject::links;
  };
  static field_setter< std::vector< std::string >, links_field > const links;

  template< typename Subject > struct linkage_field {
    static constexpr std::string Subject::* const pointer = &Subject::linkage;
  };
  static field_setter< std::string, linkage_field > const linkage;

  template< typename Subject > struct modules_field {
    static constexpr std::vector< meka::module_type > Subject::* const pointer = &Subject::modules;
  };
  static field_setter< std::vector< meka::module_type >, modules_field > const modules;

}

#endif // ifndef __MEKA_MANIP_SETTER_HPP__
