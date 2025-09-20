RAII Helpers
============

`RAII <https://en.cppreference.com/w/cpp/language/raii>`_ helpers for managing lifetime of AngelScript objects.

``asIScriptEngine*``
--------------------

.. doxygenfunction:: asbind20::make_script_engine

.. doxygenclass:: asbind20::script_engine
   :members:
   :undoc-members:

``asIScriptContext*``
---------------------

.. doxygenfunction:: asbind20::current_context

.. doxygenclass:: asbind20::request_context
   :members:
   :undoc-members:

.. doxygenclass:: asbind20::reuse_active_context
   :members:
   :undoc-members:

``asIScriptObject*``
--------------------

.. doxygenclass:: asbind20::script_object
   :members:
   :undoc-members:

``asILockableSharedBool*``
--------------------------

.. doxygenclass:: asbind20::lockable_shared_bool
   :members:
   :undoc-members:

.. doxygenfunction:: asbind20::make_lockable_shared_bool

IO Helpers
==========

The IO support is in a standalone header ``<asbind20/io/stream.hpp>``,
which means you need to include it explicitly if you want to load/save byte code of AngelScript.

.. doxygengroup:: ByteCode

.. doxygenstruct:: asbind20::io::load_byte_code_result
   :members:
   :undoc-members:

Miscellaneous Utilities
=======================

.. doxygenfunction:: asbind20::to_string(asEContextState)
.. doxygenfunction:: asbind20::to_string(asERetCodes)

.. doxygenfunction:: asbind20::string_concat
.. doxygenfunction:: asbind20::with_cstr

.. doxygenclass:: asbind20::meta::fixed_string
   :members:
   :undoc-members:

.. doxygenclass:: asbind20::compressed_pair
   :members:
   :undoc-members:

Debugging
=========

.. doxygenfunction:: asbind20::debugging::get_function_section_name

.. doxygenstruct:: asbind20::debugging::gc_statistics
   :members:
   :undoc-members:
.. doxygenfunction:: asbind20::debugging::get_gc_statistics
