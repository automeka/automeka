/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef __MEKA_MANIP_APPENDER_HPP__
#define __MEKA_MANIP_APPENDER_HPP__

#include "meka/manip/manipulable.hpp"

namespace meka {

  template<typename T, template<typename Subject> class Field>
  struct field_appender;

  template<typename T, template<typename Subject> class Field>
  struct field_appender<std::vector<T>, Field> {
    template<typename ... Ts>
    constexpr explicit field_appender(Ts&& ... ts) :
      data{std::forward<Ts>(ts) ...}
    {}

    template<typename Subject>
    constexpr operator std::function<void(Subject&)>() const
    { return [ = ](Subject & subject) { (subject.*Field<Subject>::pointer).push_back(this->data); }; }

    T const data;
  };

  template<typename Subject> struct bins_field {
    static constexpr std::vector<meka::bin_type> Subject::* const pointer = &Subject::bins;
  };

  typedef field_appender<std::vector<meka::manipulable<meka::bin_type>>, bins_field> bin;

  template<typename Subject> struct libs_field {
    static constexpr std::vector<meka::lib_type> Subject::* const pointer = &Subject::libs;
  };

  typedef field_appender<std::vector<meka::manipulable<meka::lib_type>>, libs_field> lib;

}

#endif // ifndef __MEKA_MANIP_APPENDER_HPP__
