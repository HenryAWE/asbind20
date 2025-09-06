Changelog
=========

Changelog since the version 1.5.0.

1.8.0
-----

Bug fix
~~~~~~~

- Fix error caused by generated copy constructor when registering a C array (``type[size]``) as value type.

1.7.1
-----

2025-8-7

Update
~~~~~~

- Update compatibility with AngelScript 2.38.0.
- Mark more pointers in parameter list by ``const``.
- Add CMake flag for controlling the warning-as-error compiler flag during development and test.
- Add test for clang-cl (Clang with MSVC-like interfaces) support.
- Add nightly test for checking compatibility of asbind20 with the latest WIP version of AngelScript.
- Add benchmarks for finding performance issues during development.

Bug fix
~~~~~~~

- Add some missing utility interfaces for RAII helpers.

Documentation
~~~~~~~~~~~~~

- Update and clarify.
- Fix typo.

1.7.0
-----

2025-7-19

Update
~~~~~~

- Fully support of x86 (32bit) Windows platform, including special treatment for ``__stdcall``.
- Binding generator for reference type no longer requires the registered type to be complete type.
- Policy for converting script initialization list for constructors accepting ``std::from_range(_t)`` when C++23 is enabled.

Breaking Change
~~~~~~~~~~~~~~~

- Redesign interfaces of ``container::single`` to reduce some surprising behaviors.
  For example, the destructor won't clean the data, you need to call another function for releasing the data.

Bug fix
~~~~~~~

- Fix mismatched declaration in array extension that may cause crash on x86 platform.
- Fix CMake error when configuring with Clang 19 toolchain.

Documentation
~~~~~~~~~~~~~

- Add information about using asbind20 in XMake projects.

Removed
~~~~~~~

- GCC 12 support is dropped due to too many workarounds. Please use GCC 13 or newer.

1.6.0
-----

2025-6-14

Update
~~~~~~

- Helper for registering modulo operator.
- Composite property support.
- Composite methods support including generic wrapper with or without variable types.
- Official arm64 support. Previously, it works theoretically but not actually tested.
- New tools for managing script function without increasing its reference count.
  It's useful when the programmer has deep understanding about the lifetime and want to write zero overhead code.
- Adjust file structure for maintenance.
- Slightly optimize memory usage of ``small_vector``.

Breaking Change
~~~~~~~~~~~~~~~

- Removed ``script_function_base``. Use ``script_function<void>`` instead.
- ``asbind20::meta::compressed_pair`` changed to ``asbind20::compressed_pair``
- ``asbind20::meta::overloaded`` changed to ``asbind20::overloaded``

Bug fix
~~~~~~~

- Update compatibility with ``string_view`` of binding generators.
- Fix compilation error when using an integer bigger than 8 bytes,
  such as ``__int128`` extension provided by Clang and GCC.

Documentation
~~~~~~~~~~~~~

- Fix typo and wrong link.
- Fix highlight script for AngelScript.
- Add entrance for extension library for future update. (Currently, the documents of extensions are still incomplete)

Deprecated
~~~~~~~~~~

- Support of GCC 12 will be dropped in the next release, because it requires too many workarounds. Please use GCC 13 or newer.

1.5.2
-----

2025-5-13

Update
~~~~~~

Complete support of using AngelScript interfaces within ``AngelScript::`` namespace. (Thanks `GitHub @sashi0034 <https://github.com/sashi0034>`_)

1.5.1
-----

2025-4-21

Update
~~~~~~

- Some interfaces  of ``container::single`` will return a ``bool`` value for checking result

Bug fix
~~~~~~~

- Exception guarantee for ``notify_gc`` policy
- Exception guarantee for ``small_vector``
- Fix memory leaks if any exception occurs in generated constructors / factories

1.5.0
-----

2025-3-31

What's New
~~~~~~~~~~

Core Library
^^^^^^^^^^^^

- New tools for binding complex operator overloads
- Interfaces of binding generator now all take ``std::string`` / ``std::string_view`` instead of ``const char*`` for convenience and consistency
- Tools for multithreading with AngelScript
- New tool named ``overload_cast`` for choosing desired overloaded functions

Extension Library
^^^^^^^^^^^^^^^^^

First stable version of extension library released!

Please check the comment in source code of extension for their documentation.
Full documentation for extension library is coming soon.

Documentation
~~~~~~~~~~~~~

Rewrite and migrate to Read the Docs.

Bug Fix
~~~~~~~

- Generic wrapper may crash when returning some kinds of value type by value
- Fix several bugs in ``small_vector``
