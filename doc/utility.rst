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

Miscellaneous Utilities
=======================

.. doxygenfunction:: asbind20::to_string(asEContextState)
.. doxygenfunction:: asbind20::to_string(asERetCodes)

.. doxygenfunction:: asbind20::string_concat
.. doxygenfunction:: asbind20::util::with_cstr

.. doxygenclass:: asbind20::util::fixed_string
   :members:
   :undoc-members:

.. doxygenclass:: asbind20::util::compressed_pair
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

.. doxygenfunction:: asbind20::debugging::extract_string(asIStringFactory*, const void*)
