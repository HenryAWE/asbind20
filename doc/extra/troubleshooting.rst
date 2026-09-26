Troubleshooting
===============

This page describes some common problems and their solutions.
Besides, if you encounter a runtime error that is hard to understand, you can check the following list for help.

1. Duplicate Methods/Functions
------------------------------

You may see words like ``ALREADY_REGISTERED`` being reported by the message callback. This is caused by registering a method/function for the second time.
A common cause is registering something manually after calling a helper for generating registered functions/methods/behaviours. For example, the code manually registers default constructor/copy constructor/assignment operator (``opAssign``)/destructor after using ``behaviours_by_traits()``, which will register the above behaviours.

2. Mismatched Function Signature
--------------------------------

Please check carefully that the function signatures in AngelScript and C++ match exactly. Especially when dealing with platform-dependent types, for example, ``size_t``. Besides, a common mistake is registering a function that accepts ``const char*`` / ``std::string_view`` as if it accepted ``string``. You can create a wrapper function, or register them by lambda.

3. Invalid Type Flags for Value Type
------------------------------------

AngelScript needs precise type information to use a value type in the native calling convention. Please check the official document for details. A common mistake is forgetting to set ``asOBJ_APP_CLASS_MORE_CONSTRUCTORS`` before registering constructors other than default/copy constructor.
If you think the problem is caused by the native calling convention and is hard to fix, you can try to force ``asbind20`` to register all things in generic mode, as described :ref:`here <group-force-generic>`. If switching to generic mode fixes your problem, you can be 90% sure the problem is caused by invalid type flags.

Note for Windows + MSVC Users
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you see something like "SEH exception with code 0xC0000005", try the solution in 2 and 3.
