/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef __MEKA_MANIP_MANIPULABLE_HPP__
#define __MEKA_MANIP_MANIPULABLE_HPP__

namespace meka {

  template<typename Subject>
  struct manipulable : Subject {
    typedef std::function<void (Subject&)> manipulator;
    typedef std::vector<manipulator>       manipulators;

    template<typename ... Ts>
    manipulable(Ts&& ... ts) {
      for (auto const& manipulate : manipulators { std::forward<Ts>(ts) ... })
        manipulate(*this);
    }

  };

}

#endif // ifndef __MEKA_MANIP_MANIPULABLE_HPP__
