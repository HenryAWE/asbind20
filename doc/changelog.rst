Changelog
=========

Changelog since the version 1.5.0.

1.6.0
-----

Update
~~~~~~

- Composite property support


Bug fix
~~~~~~~

- Update compatibility with ``string_view`` of binding generators.

1.5.2
-----

2025-5-13

Update
~~~~~~

Complete support of using AngelScript interfaces within ``AngelScript::`` namespace. (Thanks `GitHub @sashi0034 <https://github.com/sashi0034>`_)

1.5.1
-----

2025-4-21

Update
~~~~~~

- Some interfaces  of ``container::single`` will return a ``bool`` value for checking result

Bug fix
~~~~~~~

- Exception guarantee for ``notify_gc`` policy
- Exception guarantee for ``small_vector``
- Fix memory leaks if any exception occurs in generated constructors / factories

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
