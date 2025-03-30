Changelog
=========

Changelog since the version 1.5.0.

1.5.0
-----

What's New
~~~~~~~~~~

Core Library
^^^^^^^^^^^^

- New tools for binding complex operator overloads
- Interfaces of binding generator now all take ``std::string`` / ``std::string_view`` instead of ``const char*`` for convenience and consistency
- Tools for multithreading with AngelScript

Extension Library
^^^^^^^^^^^^^^^^^

First stable version of extension library released!

Documentation
~~~~~~~~~~~~~

Rewrite and migrate to Read the Docs.

Bug Fix
~~~~~~~

- Generic wrapper may crash when returning some kinds of value type by value
- Fix several bugs in ``small_vector``
