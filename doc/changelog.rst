Changelog
=========

Changelog since the version 1.5.0.

1.5.1
-----

Update
~~~~~~

- Some interfaces  of ``container::single`` will return a ``bool`` value for checking result

Bug fix
~~~~~~~

- Exception guarantee for ``small_vector``
- Fix memory leaks if any exception occurs in the constructors of ``ext::script_optional``

1.5.0
-----

2025-3-31

What's New
~~~~~~~~~~

Core Library
^^^^^^^^^^^^

- New tools for binding complex operator overloads
- Interfaces of binding generator now all take ``std::string`` / ``std::string_view`` instead of ``const char*`` for convenience and consistency
- Tools for multithreading with AngelScript
- New tool named ``overload_cast`` for choosing desired overloaded functions

Extension Library
^^^^^^^^^^^^^^^^^

First stable version of extension library released!

Please check the comment in source code of extension for their documentation.
Full documentation for extension library is coming soon.

Documentation
~~~~~~~~~~~~~

Rewrite and migrate to Read the Docs.

Bug Fix
~~~~~~~

- Generic wrapper may crash when returning some kinds of value type by value
- Fix several bugs in ``small_vector``
