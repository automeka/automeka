/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef __MEKA_MANIP_APPENDER_HPP__
#define __MEKA_MANIP_APPENDER_HPP__

namespace meka {

  template< typename T, template< typename S > class D >
  struct field_appender;

  template< typename T, template< typename S > class D >
  struct field_appender< std::vector< T >, D > {
    template< typename ... Ts >
    explicit constexpr field_appender(Ts&& ... ts) :
      data { std::forward< Ts >(ts) ... }
    {}

    template< typename S >
    constexpr operator std::function< void(S&) >() const { return [ = ](S & subject)->void { (subject.*D< S >::pointer).push_back(this->data); }; }

    T data;
  };

  template< typename S > struct bins_field {
    static constexpr std::vector< meka::bin_type > S::* pointer = &S::bins;
  };
  typedef field_appender< std::vector< meka::bin_type >, bins_field > bin;

  template< typename S > struct libs_field {
    static constexpr std::vector< meka::lib_type > S::* pointer = &S::libs;
  };
  typedef field_appender< std::vector< meka::lib_type >, libs_field > lib;

}

#endif // ifndef __MEKA_MANIP_APPENDER_HPP__
