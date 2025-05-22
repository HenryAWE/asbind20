Extension for String and Char
=============================

Before registering the string and char extension,
it is recommended that to set the required engine properties.

.. doxygenfunction:: asbind20::ext::configure_engine_for_ext_string

Script Char
-----------

.. doxygenfunction:: asbind20::ext::register_script_char

This module simply wraps the primitive C++ type ``char32_t`` as script type ``char``.

It can also serve as a demonstration for registering a primitive C++ type using ``value_class``.

Script String
-------------

This module optionally depends on the script char and the array extension,
so it is recommended to register them before the script string.
However, those interfaces will be automatically disabled if those dependencies are not found.

.. doxygenfunction:: asbind20::ext::register_std_string
.. doxygenfunction:: asbind20::ext::register_string_utils

C++ Interface
~~~~~~~~~~~~~

The C++ interface is the ``std::string`` from the standard library with a UTF-8 layer on top of it.

The UTF-8 Library
^^^^^^^^^^^^^^^^^

.. doxygengroup:: UTF-8

.. doxygenclass:: asbind20::ext::utf8::const_string_iterator
   :members:
   :undoc-members:

.. doxygenfunction:: asbind20::ext::utf8::string_cbegin
.. doxygenfunction:: asbind20::ext::utf8::string_cend
