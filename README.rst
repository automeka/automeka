===============================================
|Automeka| C++1y automated make
===============================================
.. |Automeka| image:: https://avatars2.githubusercontent.com/u/5724814?s=128
    :alt: Automkea
    :target: https://github.com/automeka/automeka

.. image:: http://img.shields.io/travis/automeka/automeka/master.svg?style=flat-square
    :alt: Build Status
    :target: https://travis-ci.org/automeka/automeka

.. image:: http://img.shields.io/github/release/automeka/automeka.svg?style=flat-square
    :alt: Github Release
    :target: https://github.com/automeka/automeka/releases

.. image:: http://img.shields.io/badge/license-UNLICENSE-brightgreen.svg?style=flat-square
    :alt: Unlicense
    :target: http://unlicense.org/UNLICENSE

.. image:: http://img.shields.io/badge/gitter-join%20chat%20%E2%86%92-brightgreen.svg?style=flat-square
    :alt: Gitter Chat
    :target: https://gitter.im/automeka/automeka


USAGE
````````````````````````````
Automeka automatically finds and builds your projects without any configuration. Automeka assumes
that the source tree represents the logical structure of projects.

Automeka exclusively uses Clang for compilation for now, as it requires the not-yet-standardized C++ modules.

1. Any folder containing a ``src`` or an ``include`` folder is considered as a module named after
   the folder.

2. Regarding include paths, ``include`` folder is assumed to contain the public interface of the
   module. The ``src`` folder is assumed to contain the private interface of the module. [1]_

3. Each source file found in the ``src`` folder of a module is compiled into an object.

4. Each object is then scanned for a main function. A library is created for each module,
   with all the objects that do not contain a main function.

5. For each object with a main function, an executable is created and linked against all the
   module's libraries it depends on. [2]_

.. [1] Each module should provide a ``module.modulemap`` file in its ``include`` folder, describing
   the module's headers and libraries to link against.

.. [2] Libraries to link against are described in the module's ``module.modulemap`` file. See
   http://clang.llvm.org/docs/Modules.html for more information.


BUILD
````````````````````````````
Automeka currently uses Ninja for resolving build order and executing tasks. The goal is not to
rely on some external make program, and this should be internalized.

Ninja is also required for bootstraping Automeka, when no precursor is available.

.. code:: bash

  # bootstrap automeka
  ninja

  # rebuild automeka with itself and C++ modules enabled
  ./build/automeka


LICENSE
````````````````````````````

 This is free and unencumbered software released into the public domain.

 See accompanying file UNLICENSE or copy at http://unlicense.org/UNLICENSE
