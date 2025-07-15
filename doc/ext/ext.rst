Extension Library
=================

The extension library contains some tools that are less flexible than those in the core library.
It also contains demonstration for how to register some common types like array or string by asbind20.

All interfaces of extension library is under the ``asbind20::ext`` namespace.
There source code are located inside the ``ext/`` directory of repository.

.. toctree::
   :maxdepth: 1
   :caption: Container Library

   container/array
   container/stdstring

.. note::
   CMake target name is ``asbind20_ext_container``.

.. toctree::
   :maxdepth: 2
   :caption: Utilities

   utility/math
   utility/vocabulary
   utility/misc

.. note::
   CMake target name is ``asbind20_ext_utility``.
