===============================================
Automeka: C++14 automated make prototype.
===============================================
.. image:: http://img.shields.io/travis/berenm/meka/automeka.svg?style=flat-square
    :alt: Build Status
    :target: https://travis-ci.org/berenm/meka

.. image:: http://img.shields.io/github/issues/berenm/meka.svg?style=flat-square
    :alt: Github Issues
    :target: https://github.com/berenm/meka/issues

.. image:: http://img.shields.io/github/release/berenm/meka.svg?style=flat-square
    :alt: Github Release
    :target: https://github.com/berenm/meka/releases

USAGE
````````````````````````````
Automeka automatically finds and builds your projects without any configuration. Automeka assumes
that the source tree represents the logical structure of projects.

1. Any folder containing a ``src`` or an ``include`` folder is considered as a project named after
   the folder.

2. Regarding include paths, ``include`` folder is assumed to contain the public interface of the
   project. The ``src`` folder is assumed to contain the private interface of the project.

3. Each source file found in the ``src`` folder of a project is compiled into an object.

4. Each object is then scanned for a main function. A static library is created for each project,
   with all the objects that do not contain a main function.

5. For each object with a main function, an executable is created and linked against all the static
   libraries.

BUILD
````````````````````````````
Automeka currently uses Ninja for dependency managment. The goal is not to rely on some external
make program, and this should be internalized. However Ninja is also required for bootstraping
Automeka, when no precursor is available.

.. code:: bash

  ninja
  ./build/automeka


COPYING INFORMATION
````````````````````````````

 Distributed under the Boost Software License, Version 1.0.

 See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
