Array Extension
===============

Configuring User Data ID
------------------------

By default, the array extension will use **2000** as the ID.

You can define the macro ``ASBIND20_EXT_SCRIPT_ARRAY_USER_ID`` to customize this value.
Please note that this ID cannot overlap with another user data ID.

Registering
-----------

The array extension does not have any dependencies.

.. doxygenfunction:: asbind20::ext::register_script_array
