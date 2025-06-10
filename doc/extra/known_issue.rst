Known Issues
============

Some feature of this library may not work on some broken compiler or standard library.

1. If you bind an overloaded function using syntax like ``static_cast<return_type(*)()>(&func)`` on MSVC, it may crash the compiler.
   The workaround is to write a standalone wrapper function with no overloading, then bind this wrapper using asbind20. You can also try the ``overload_cast`` tool.

2. GCC (at least for 14.2) may `not generate machine code of registered template class on a rare occasion <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119233>`_. If your encounter a linker error like `this example on Compiler Explorer <https://godbolt.org/z/oT9cP1rso>`_ , try to use explicit instantiation for your template class.

3. Older compilers (like GCC 12) may wrongly think some types in asbind20 are dependent types, so you need to add ``template`` disambiguator as a workaround.
