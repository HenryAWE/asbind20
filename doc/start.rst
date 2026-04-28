Prerequisites
=============

- AngelScript 2.37.0 or newer.
- Any C++ compiler that supports C++20 features required by this library.
  This library mainly depends on ``requires`` and ``<concepts>`` from C++20.

If the compiler and standard library support newer features like C++23,
asbind20 will enable support for them.

There is a nightly build script in CI of asbind20,
testing its compatibility with `the latest WIP version of AngelScript <https://www.angelcode.com/angelscript/wip.php>`_.
If you are using a WIP version of AS for newest features or fixes,
you need to consider using the latest release of asbind20,
or even the development branch.

Supported Platforms
===================

Currently, the following platforms and compilers are officially supported and `tested by CI <https://github.com/HenryAWE/asbind20/blob/master/.github/workflows/build.yml>`_.
However, asbind20 on other platforms, architectures or compiler toolchains supported by AngelScript should work as expected too.

.. list-table::
   :widths: auto
   :header-rows: 1

   * - Platform
     - Architecture
     - Compiler
   * - Windows
     - x86, x64
     - MSVC latest
   * - Windows
     - x64
     - Clang-Cl latest
   * - Linux (Ubuntu)
     - x64
     - | GCC 13, 14
       | Clang 15, 18, 19
   * - Linux (Ubuntu)
     - arm64
     - Clang 18
   * - MacOS
     - arm64
     - Xcode latest-stable
   * - Emscripten
     - wasm
     - emsdk 4.0.18

- The "latest" or "latest-stable" in the above table mean the (stable) latest version of compiler toolchain available in CI when asbind20 releases a new stable version.
  You can check the release date of historical versions in the :doc:`changelog`.

- Newer Clang toolchains (like Clang 18+) support both libstdc++ and libc++,
  while the older toolchains (like Clang 15) on libstdc++ are not guaranteed to work as expected.

- Additionally, the Clang 18 toolchain on x64 Linux is tested for compiling with address sanitizer and ``AS_USE_NAMESPACE`` enabled.

.. note::
  This library on older compiler toolchains, like GCC 12, still works as expected under most situations,
  but it might need some workarounds to deal with defects of the compiler, e.g., additional ``typename`` in template-related code.
  Please use newer compiler toolchains if possible.

Integrate into Your Project
===========================

Please follow the `tutorial of AngelScript to build and install it <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_compile_lib.html>`_ at first,
or use a package manager like `vcpkg <https://github.com/microsoft/vcpkg>`_.

Besides, if your project has a custom location of ``<angelscript.h>``, you can include it before any asbind20 headers.
This library will not include the AngelScript library for the second time by detecting the macro ``ANGELSCRIPT_H``.

1. Copy into Your Project
-------------------------

This is a header-only library.
You can directly copy all the files under ``include/`` into your project,
then set a correct include directory for your project.

2. Integration Using CMake
--------------------------

CMake 3.20 or newer is required.

A. Submodule
~~~~~~~~~~~~

You can add asbind20 through git submodule and then use ``add_subdirectory``.

.. code-block:: cmake

    add_subdirectory("directory/of/asbind20")

    target_link_libraries(main PRIVATE asbind20::asbind20)

B. Build and Install
~~~~~~~~~~~~~~~~~~~~

Additionally, you can install and link against asbind20 using CMake.

.. code-block:: sh

    cmake -S . -B build
    cmake --install build

Then, link the library in a ``CMakeLists.txt``.

.. code-block:: cmake

    find_package(asbind20 REQUIRED)

    target_link_libraries(main PRIVATE asbind20::asbind20)

1. Integration with XMake Projects
----------------------------------

Since the version 1.6.0, asbind20 has been accepted into the `official package registry (xmake-repo) <https://github.com/xmake-io/xmake-repo>`_.
You can easily add asbind20 as a dependency of your project.

.. code-block:: lua

    add_requires("asbind20")

    target("main")
        -- ... --
        add_packages("asbind20")

Please check the `official documentation of XMake about adding packages <https://xmake.io/guide/project-configuration/add-packages.html>`_ for more details.
