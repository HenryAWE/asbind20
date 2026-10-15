Formatting Backends
===================

Several asbind20 APIs turn AngelScript types into human-readable strings:
``to_string`` / ``to_wstring``, the formatter specializations for AngelScript
enumerators and script stack traces, the formatted logging helpers, and the
bytecode dumper.
They all go through a *formatting backend*, which is selected at compile time:

- The C++20 standard library ``<format>``, used by default when it is available.
- The `{fmt} <https://fmt.dev/>`_ library, enabled explicitly.

Whichever backend is active, asbind20 refers to it through the
``asbind20::io::fmtlib`` namespace, so the same call compiles with either backend:

.. code-block:: c++

    #include <asbind20/io/formatter.hpp>

    std::string name = asbind20::io::fmtlib::format("{}", asTC_IDENTIFIER);

``asbind20::io::fmtlib`` provides ``format_string``, ``format``, ``format_to``,
``format_args``, ``make_format_args``, ``vformat`` and ``format_error``,
mirroring the interface of the selected library.

Selecting a Backend
-------------------

By default, asbind20 uses ``std::format`` when the standard library provides
``<format>`` (that is, when ``__cpp_lib_format`` is defined).
To use `{fmt}` instead, define ``ASBIND20_USE_FMTLIB`` before including any
asbind20 header, and link your project against `{fmt}`:

.. code-block:: c++

    #define ASBIND20_USE_FMTLIB 1
    #include <asbind20/asbind.hpp>

The backend includes ``<fmt/format.h>`` and ``<fmt/xchar.h>``; the latter is
needed for wide-character output such as ``to_wstring``.

When asbind20 is added to a CMake project, the ``asbind_use_fmtlib`` option
(``OFF`` by default) does both steps for you: it defines the macro and links
``fmt::fmt``, and the setting is propagated to the targets linking
``asbind20::asbind20``.

.. code-block:: cmake

    set(asbind_use_fmtlib ON)   # or configure with -Dasbind_use_fmtlib=ON

    add_subdirectory("directory/of/asbind20")

    target_link_libraries(main PRIVATE asbind20::asbind20)

.. note::

    ``ASBIND20_USE_FMTLIB`` and ``<format>`` are mutually exclusive in asbind20:
    when the macro is defined, asbind20 registers ``fmt::formatter``
    specializations for its types and does **not** register the ``std::formatter``
    ones, even if ``<format>`` is available.
    If your project uses both libraries, keep the default backend: asbind20 types
    then stay formattable with ``std::format`` and are not formattable with
    ``fmt::format``.

If neither backend is available, asbind20 still builds: ``to_string`` and
``to_wstring`` fall back to ``std::to_string``-based output, and no formatter
specializations are registered.

Detecting the Backend
---------------------

``ASBIND20_HAS_FORMAT_LIBRARY`` is defined by ``<asbind20/io/fmtlib.hpp>`` when a
backend is available.
Its value is the string literal ``"<format>"`` or ``"{fmt}"``, which can be
reported directly:

.. code-block:: c++

    #include <asbind20/io/fmtlib.hpp>

    #ifdef ASBIND20_HAS_FORMAT_LIBRARY
    auto msg = asbind20::io::fmtlib::format(
        "formatting with {}", ASBIND20_HAS_FORMAT_LIBRARY
    );
    #endif

Formattable Types
-----------------

asbind20 registers formatters for the following types:

.. list-table::
  :widths: auto
  :header-rows: 1

  * - Type
    - Header
  * - ``asEContextState``, ``asERetCodes``, ``asEMsgType``, ``asETokenClass``, ``asEEngineProp``
    - ``<asbind20/io/formatter.hpp>``
  * - ``asbind20::cstring_ref``, ``asbind20::util::fixed_string``
    - ``<asbind20/util/strutil.hpp>``
  * - ``asbind20::debugging::stacktrace_entry``, ``asbind20::debugging::basic_stacktrace``
    - ``<asbind20/debugging/stacktrace.hpp>``

The AngelScript enumerators accept the following presentation types:

.. list-table::
  :widths: auto
  :header-rows: 1

  * - Specifier
    - Result
  * - ``{}``
    - The enumerator name without its ``as`` prefix, e.g. ``IDENTIFIER``
  * - ``{:?}``
    - The full enumerator name, e.g. ``asTC_IDENTIFIER``
  * - ``{:d}``
    - The underlying integer value
  * - ``{:x}``
    - The underlying integer value in hexadecimal

If a value does not match any enumerator of its type, the output falls back to
``TypeName(value)``, e.g. ``asEEngineProp(-1)``; both backends produce the same text.

The common fill, alignment and width specifiers are supported as well, and are
applied to the result of every presentation type.
The enumerator names are left-aligned by default, like strings, while the
underlying integers are right-aligned by default, like numbers:

.. code-block:: c++

    using namespace asbind20;

    io::fmtlib::format("{}", asTC_IDENTIFIER);        // "IDENTIFIER"
    io::fmtlib::format("{:?}", asTC_IDENTIFIER);      // "asTC_IDENTIFIER"
    io::fmtlib::format("{:d}", asTC_IDENTIFIER);      // the underlying integer
    io::fmtlib::format("{:*>12}", asTC_IDENTIFIER);   // "**IDENTIFIER"
    io::fmtlib::format("{:8d}", asEP_ALLOW_UNSAFE_REFERENCES); // "       1"

The width can also be given by a nested replacement field, e.g.
``io::fmtlib::format("{:{}}", asTC_IDENTIFIER, 12)``.

.. note::

    The stack trace formatters currently support ``char`` output only, so
    ``stacktrace_entry`` and ``basic_stacktrace`` cannot be formatted into a
    ``std::wstring`` or with a wide format string.

Related APIs
------------

- ``asbind20::to_string`` and ``asbind20::to_wstring`` return the same
  representations as shown above; see :doc:`utility`.
- ``asbind20::io::copy_debug_representation_to`` writes a representation to an
  output iterator instead of a string, which is convenient for stream-based code.
  It is declared in ``<asbind20/io/formatter.hpp>``.
- ``asbind20::io::fmtlib::basic_format_data`` and its ``parse`` member function
  implement the common fill, alignment and width specifiers, and can be reused
  by custom formatters. It is declared in ``<asbind20/io/fmtlib.hpp>``.
- :ref:`Formatted logging <formatted-logging>` and the bytecode dumper described
  in :doc:`utility` use the selected backend as well.
