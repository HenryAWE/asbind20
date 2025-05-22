Weak Reference for Script
=========================

C++ Interfaces
--------------

.. doxygenclass:: asbind20::ext::script_weakref
   :members:
   :undoc-members:

Example Usage
-------------

.. code-block:: AngelScript

    class test{};

    void some_function()
    {
        test@ t = test();
        weakref<test> r(t);

        // ...
    }
