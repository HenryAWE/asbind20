RAII Helpers
============

`RAII <https://en.cppreference.com/w/cpp/language/raii>`_ helpers for managing lifetime of AngelScript objects.

Script Engine
-------------

.. doxygenfunction:: asbind20::make_script_engine

.. doxygenclass:: asbind20::script_engine
  :members:
  :undoc-members:

Script Context
--------------

.. doxygenfunction:: asbind20::current_context

.. doxygenclass:: asbind20::request_context
  :members:
  :undoc-members:

.. doxygenclass:: asbind20::reuse_active_context
  :members:
  :undoc-members:

Script Object
-------------

.. doxygenclass:: asbind20::script_object
  :members:
  :undoc-members:

Script Type Information
-----------------------

.. doxygenclass:: asbind20::script_typeinfo
  :members:
  :undoc-members:


Lockable Shared Bool
--------------------

.. doxygenclass:: asbind20::lockable_shared_bool
  :members:
  :undoc-members:

.. doxygenfunction:: asbind20::make_lockable_shared_bool

IO Helpers
==========

.. doxygengroup:: ByteCode

.. doxygenstruct:: asbind20::io::load_byte_code_result
  :members:
  :undoc-members:

Loading Script Sections
-----------------------

.. doxygenfunction:: asbind20::io::load_string
.. doxygenfunction:: asbind20::io::load_file


Miscellaneous Utilities
=======================

.. doxygenfunction:: asbind20::to_string(asEContextState)
.. doxygenfunction:: asbind20::to_string(asERetCodes)

.. doxygenfunction:: asbind20::string_concat

Example:

.. code-block:: c++

    using asbind20::string_concat;

    // Concatenate any mix of string types
    const char* method = "my_method";
    std::string decl = string_concat("void f(", method, ')');
    assert(decl == "void f(my_method)");

    // Accepts std::string_view, cstring_ref, and characters
    std::string s = string_concat("hello", ' ', std::string_view("world"));
    assert(s == "hello world");

.. doxygenfunction:: asbind20::util::with_cstr

``with_cstr`` calls a callback with each argument converted to a null-terminated C string,
handling any string-like type including ``std::string``, ``std::string_view``,
``cstring_ref``, and ``util::fixed_string``:

.. code-block:: c++

    using asbind20::util::with_cstr;
    using namespace std::string_view_literals;

    std::string result = with_cstr(
        [](const char* a, const char* b, const char* c) {
            return std::string(a) + ' ' + b + ' ' + c;
        },
        "hello",
        "world"sv.substr(0, 3),  // "wor"
        asbind20::cstring_ref("!")
    );
    assert(result == "hello wor !");

.. doxygenclass:: asbind20::util::fixed_string
  :members:
  :undoc-members:

.. doxygenclass:: asbind20::util::compressed_pair
  :members:
  :undoc-members:

Example:

.. code-block:: c++

    using asbind20::util::compressed_pair;

    compressed_pair<int, int> p{0, 1};
    assert(p.first() == 0);
    assert(p.second() == 1);

    p.first() = 2;
    p.second() = 3;

    // Structured binding support
    auto& [a, b] = p;
    assert(a == 2);
    assert(b == 3);

    // Swap
    compressed_pair<int, int> q{4, 5};
    p.swap(q);
    assert(p.first() == 4 && p.second() == 5);

Atomic Reference Counting
-------------------------

``atomic_counter`` wraps AngelScript's ``asAtomicInc`` / ``asAtomicDec`` for thread-safe reference counting.
It is designed for implementing the ``addref`` / ``release`` / ``get_refcount`` behaviours of a garbage-collected
``ref_class``.

Constructing an ``atomic_counter`` initializes the counter to 1. Prefix ``++`` and ``--`` call ``asAtomicInc``
and ``asAtomicDec`` respectively, returning the new value. The ``dec_and_try_destroy`` method decrements and,
if the counter reaches 0, invokes a destroyer callback — typically ``delete`` on the enclosing object.

Example: implementing reference counting for a garbage-collected script object:

.. code-block:: c++

    #include <asbind20/asbind.hpp>

    class my_gc_object
    {
    public:
        void addref()
        {
            set_gc_flag(false);
            ++m_counter;
        }

        void release()
        {
            m_counter.dec_and_try_destroy(
                [](my_gc_object* self) { delete self; },
                this
            );
        }

        int get_refcount() const
        {
            return m_counter;
        }

        void set_gc_flag()  { m_flag = true; }
        bool get_gc_flag() const { return m_flag; }

        void enum_refs(asIScriptEngine*)  { /* enumerate references */ }
        void release_refs(asIScriptEngine*)  { /* release references */ }

    private:
        asbind20::atomic_counter m_counter;
        bool m_flag = false;
    };

    // Register as a GC reference type
    auto* engine = /* ... */;
    asbind20::ref_class<my_gc_object>(engine, "my_gc_object", asOBJ_GC)
        .addref(fp<&my_gc_object::addref>)
        .release(fp<&my_gc_object::release>)
        .get_refcount(fp<&my_gc_object::get_refcount>)
        .set_gc_flag(fp<&my_gc_object::set_gc_flag>)
        .get_gc_flag(fp<&my_gc_object::get_gc_flag>)
        .release_refs(fp<&my_gc_object::release_refs>)
        .enum_refs(fp<&my_gc_object::enum_refs>)
        .default_factory(use_policy<policies::notify_gc>);

See :ref:`the threading documentation <atomic-refcounting>` for the full API reference.

Range Views
-----------

asbind20 provides range views for iterating over AngelScript entities — type members, behaviours, enum values,
and even tokenized script source. When ``<ranges>`` is available (``__cpp_lib_ranges``), these views integrate with
standard range adaptors and algorithms.

All views expose ``begin()``/``end()``, ``size()``, and ``empty()``. Most iterators are random-access.

Type information views
~~~~~~~~~~~~~~~~~~~~~~

These views wrap an ``asITypeInfo*`` and enumerate its contents:

.. code-block:: c++

    #include <asbind20/ranges.hpp>

    const asITypeInfo* ti = /* ... */;

    // Iterate all methods (optionally exclude virtual methods)
    for(auto* func : asbind20::views::all_methods(ti)) {
        std::cout << func->GetName() << '\n';
    }

    // Iterate all behaviours (constructors, destructors, opAssign, etc.)
    for(auto [beh, func] : asbind20::views::all_behaviours(ti)) {
        std::cout << "behaviour " << beh << ": " << func->GetName() << '\n';
    }

    // Iterate all factories
    for(auto* func : asbind20::views::all_factories(ti)) {
        std::cout << func->GetName() << '\n';
    }

    // Iterate all enum values
    for(auto [name, value] : asbind20::views::all_enum_values(ti)) {
        std::cout << name << " = " << value << '\n';
    }

The ``all_enum_values`` factory defaults to ``int`` as the underlying type.
Use ``views::all_enum_values_of<std::uint64_t>(ti)`` for enums with a custom underlying type.

.. doxygenclass:: asbind20::ranges::all_methods_view
  :members:
  :undoc-members:

.. doxygenclass:: asbind20::ranges::all_behaviours_view
  :members:
  :undoc-members:

.. doxygenclass:: asbind20::ranges::all_factories_view
  :members:
  :undoc-members:

.. doxygenclass:: asbind20::ranges::all_enum_values_view
  :members:
  :undoc-members:

Tokenize view
~~~~~~~~~~~~~

``tokenize_view`` tokenizes a string of AngelScript source code lazily, yielding
``(asETokenClass, std::string_view)`` pairs:

.. code-block:: c++

    #include <asbind20/ranges.hpp>

    std::string_view code = "int foo = 42;";
    for(auto [tc, sv] : asbind20::ranges::tokenize_view(engine, code)) {
        if(tc == asTC_IDENTIFIER)
            std::cout << "identifier: " << sv << '\n';
    }

The view is an input range — it does not support random access or multi-pass iteration.

.. doxygenclass:: asbind20::ranges::tokenize_view
  :members:
  :undoc-members:

Debugging
=========

.. doxygenfunction:: asbind20::debugging::get_function_section_name

GC Statistics
-------------

.. doxygenstruct:: asbind20::debugging::gc_statistics
  :members:
  :undoc-members:
.. doxygenfunction:: asbind20::debugging::get_gc_statistics

String Extraction
-----------------

Tools for extracting string from script without knowing its underlying type.

.. doxygenclass:: asbind20::debugging::extract_string_result
    :members:
    :undoc-members:

.. doxygenfunction:: asbind20::debugging::extract_string(const asIStringFactory*, const void*)
