Registering Template Types
==========================

Constructors and Factories
--------------------------

AngelScript will pass a hidden ``asITypeInfo*`` in the front of arguments to indicating an instantiation fo template.
It will be automatically handled by asbind with the special binding generators of templated types,
which are ``template_value_class`` and ``template_ref_class``.

.. note::
   ``asGetTypeTraits<T>()`` cannot determine the type flags for a template value type,
   because its constructors contain parameters of the hidden ``asITypeInfo*``.

   As a result, you need to manually pass the flags to the binding generator.
   This also applies to the ``.behaviours_by_traits()`` helper.

Here are examples from the extension library.

.. code-block:: c++

    script_array::script_array(asITypeInfo* ti)
    { /* ... */ }

    script_array::script_array(asITypeInfo* ti, asUINT n)
    { /* ... */ }

.. code-block:: c++

    using namespace asbind20;

    template_value_class<script_optional>(
        engine,
        "optional<T>",
        asOBJ_GC | asOBJ_APP_CLASS_CDAK | asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    );

    template_ref_class<array_t, UseGeneric>(
        engine,
        "array<T>",
        asOBJ_GC
    )
        .default_factory() // script_array(asITypeInfo*)
        .factory<asUINT>("uint", use_explicit) // script_array(asITypeInfo*, asUINT)

Methods of a Template Class
---------------------------

Methods are registered the same way of ordinary types.
You can directly use the subtype in the declaration of method.

.. code-block:: c++

    using namespace asbind20;
    template_ref_class<script_array>(/* ... */)
        .factory<size_type, const void*>("uint n, const T&in value", use_policy<policies::notify_gc>)
        .list_factory("repeat T", use_policy<policies::repeat_list_proxy, policies::notify_gc>)
        .method("T& opIndex(uint)", &script_array::opIndex)
        .method("const T& opIndex(uint) const", &script_array::opIndex);

.. note::
   If the subtype might be an object handle,
   you probably need to consider the keyword ``if_handle_then_const``.

   You can also read `official document explaining this <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_adv_template.html#doc_adv_template_1_1>`_

   .. code-block:: c++

    using namespace asbind20;
    template_ref_class<script_array>(/* ... */)
        .method("array_iterator<T> find(const T&in if_handle_then_const value, int start=0, uint n=uint(-1))". /* ... */);

Callback for Validating Template Instantiations
===============================================

In order to avoid unnecessary runtime validations of invalid template instantiations,
the application should preferably register a validator by ``.template_callback``.
Its underlying type behavior is ``asBEHAVE_TEMPLATE_CALLBACK``.

The callback function must be a global function that receives a pointer to ``asITypeInfo*`` as its first parameter,
and should return a boolean.
If the template instance is valid the return value should be ``true``.

The function should also take a second parameter with an output reference to a boolean.
This parameter should be set to ``true`` by the function if the template instance should not be garbage collected,
which will make AngelScript clear the ``asOBJ_GC`` flag for the object type.
If the template instance cannot form any circular references, then it doesn't need to be garbage collected,
which reduces the work that has to be done by the garbage collector.

:doc:`The tools for handling AngelScript types can help you to deal with validation. <advanced/script_type>`

Example code from the validator of ``script_optional``:

.. code-block:: c++

    static bool optional_template_callback(
        asITypeInfo* ti, bool& no_gc
    )
    {
        int subtype_id = ti->GetSubTypeId();
        if(is_void_type(subtype_id))
            return false;

        if(is_primitive_type(subtype_id))
            no_gc = true;
        else
            no_gc = !type_requires_gc(ti->GetSubType());

        return true;
    }

.. code-block:: c++

    template_value_class<script_optional>(/* .... */)
        .template_callback(&optional_template_callback);

Template Specializations
========================

When registering a template specialization,
you override the template instance that AngelScript would normally do when compiling a declaration with the template type.
This allow the application to register a completely different object with its own implementation for template specializations.

Template specializations are registered like :doc:`ordinary types <object_type>`,
except the type name should be the instantiated name.

.. code-block:: c++

    value_class<vec2<float>>(engine, "vec2<float>", /* ... */)
        .method(/* ... */);
