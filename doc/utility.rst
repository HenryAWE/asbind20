RAII Utilities
==============

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

``asILockableSharedBool*``
--------------------------

.. doxygenclass:: asbind20::lockable_shared_bool
   :members:
   :undoc-members:

Multithreading Utilities
========================

Locks
-----

.. doxygenvariable:: asbind20::as_shared_lock
.. doxygenvariable:: asbind20::as_exclusive_lock

The weak reference flag (``asILockableSharedBool*``) is also lockable.

.. doxygenclass:: asbind20::lockable_shared_bool
   :members: lock, unlock
   :no-link:

Example code:

.. code-block:: c++

    {
        std::lock_guard guard(asbind20::as_exclusive_lock);
        // Do something...
    }


Miscellaneous Utilities
=======================

.. doxygenfunction:: asbind20::to_string(asEContextState)
.. doxygenfunction:: asbind20::to_string(asERetCodes)

.. doxygenfunction:: asbind20::string_concat
.. doxygenfunction:: asbind20::with_cstr
