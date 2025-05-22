Script Math Support
===================

Basic Math Functions
--------------------

.. doxygenfunction:: asbind20::ext::register_math_function

This function will register math functions from ``<cmath>`` and ``<numeric>``,
as well as some additional helpers.

.. doxygenfunction:: asbind20::ext::math_close_to


Math Constants
--------------

.. doxygenfunction:: asbind20::ext::register_math_constants

Complex Number
--------------

Register the ``std::complex<T>``, where ``T`` can be ``float`` or ``double``.

.. doxygenfunction:: asbind20::ext::register_math_complex
