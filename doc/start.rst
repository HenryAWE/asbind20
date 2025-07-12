Supported Platforms
===================

- CMake >= 3.20
- AngelScript >= 2.37.0
- C++ compiler that supports C++20

Currently, the following platforms and compilers are officially supported and tested by CI.

.. list-table::
   :widths: auto
   :header-rows: 1

   * - Platform
     - Compiler
   * - | Windows x64
       | Windows x86
     - MSVC 19.41+
   * - Linux x64
     - | GCC 13, 14
       | Clang 18
       | Clang 18 (ASan)
   * - Linux arm64
     - Clang 18
   * - Emscripten
     - emsdk latest

.. note::
   Some advanced features are only available on newer compilers.

Integrate into Your Project
===========================

Please follow the `tutorial of AngelScript to build and install it <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_compile_lib.html>`_ at first,
or use a package manager like `vcpkg <https://github.com/microsoft/vcpkg>`_.

You can also find example script for installing AngelScript in `the GitHub Actions script of asbind20 <https://github.com/HenryAWE/asbind20/blob/master/.github/workflows/build.yml>`_.

A. Copy into Your Project
-------------------------

The core library of asbind20 is a header-only library.
You can directly copy all the files under ``include/`` into your project.

If your project has a custom location of ``<angelscript.h>``, you can include it before any asbind20 headers.
This library will not include the AngelScript library for the second time.

Additionally, files under ``io/``, ``container/``, and ``concurrent/`` are optional components.
You can omit those files if they are unnecessary for your project.

.. note::
   **About The Extension Library** (``asbind20::ext``)

   The extension part of library (under ``ext/``) is not header-only.
   If you wish to use this part of library without building asbind20 separately,
   you need to copy necessary source files as well.

B. Build and Install Using CMake
--------------------------------

.. code-block:: sh

    cmake -GNinja -DCMAKE_BUILD_TYPE=Release -S . -B build
    cmake --build build
    cmake --install build

Link the library in a ``CMakeLists.txt``.

.. code-block:: cmake

    find_package(asbind20 REQUIRED)

    target_link_libraries(main PRIVATE asbind20::asbind20)

C. Integration with XMake Projects
----------------------------------

Since the version 1.6.0, asbind20 has been accepted into the `official package registry <https://github.com/xmake-io/xmake-repo>`_.
You can easily add asbind20 as a dependency of your project.

.. code-block:: lua

    add_requires("asbind20")

    target("main")
        -- ... --
        add_packages("asbind20")
