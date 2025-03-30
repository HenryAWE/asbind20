Storing Script Objects
======================

Sometimes you may want to store script objects and use them later,
or to implement your own container for script objects.
There are some tools provided by asbind20 for storing script objects.

Storing a Single Script Object
------------------------------

.. doxygenclass:: asbind20::container::single
   :members:
   :undoc-members:

|script_optional|_.

.. |script_optional| replace:: You can check the ``script_optional`` from extension library as an example
.. _script_optional: https://github.com/HenryAWE/asbind20/blob/master/ext/utility/src/vocabulary.cpp

Containers
----------

Policies of Type Information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Policies for how containers deal with the type information.

.. doxygenconcept:: asbind20::container::typeinfo_policy

.. doxygengroup:: TypeInfoPolicies
   :content-only:

Sequential Containers
~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: asbind20::container::small_vector
   :members:
   :undoc-members:

|script_array|_.

.. |script_array| replace:: You can check the ``script_array`` from extension library as an example
.. _script_array: https://github.com/HenryAWE/asbind20/blob/master/ext/container/include/asbind20/ext/array.hpp


Associative Containers
~~~~~~~~~~~~~~~~~~~~~~

*Not implemented yet*
