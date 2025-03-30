Garbage Collected Types
=======================

Registering GC Types
--------------------

GC Type Flag
~~~~~~~~~~~~

Add the flag ``asOBJ_GC`` to the binding generator to mark the type as garbage collected.

.. code-block:: c++

    asbind20::value_class<my_value_class>(
        engine, "my_value_class", other_flags | asOBJ_GC
    );
    asbind20::ref_class<my_ref_class>(
        engine, "my_value_class", asOBJ_GC
    );

GC Behaviors of Reference Types
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The process of determining the dead objects uses the first four of the behaviours below,
while the destruction of the objects is done by forcing the release of the object's references.

Example implementation of a garbage collected type:

.. code-block:: c++

    class gc_type
    {
    public:
        void set_gc_flag()
        {
            m_gc_flag = true;
        }

        bool get_gc_flag() const
        {
            return m_gc_flag;
        }

        int get_refcount() const
        {
            return m_refcount;
        }

        void enum_refs(asIScriptEngine* engine)
        {
            engine->GCEnumCallback(m_ref);
        }

        void release_refs(asIScriptEngine* engine)
        {
            if(m_ref)
            {
                m_ref->Release();
                m_ref = nullptr;
            }
        }

        // ...

    private:
        int m_refcount = 1;
        bool m_gc_flag = false;

        asIScriptObject* m_ref = nullptr;
    };

Registering the GC behaviors:

.. code-block:: c++

    asbind20::ref_class<gc_type>(engine, "gc_type", asOBJ_GC)
        // asBEHAVE_SETGCFLAG
        .set_gc_flag(fp<&gc_type::set_gc_flag>)
        // asBEHAVE_GETGCFLAG
        .get_gc_flag(fp<&gc_type::get_gc_flag>)
        // asBEHAVE_GETREFCOUNT
        .get_refcount(fp<&gc_type::get_refcount>)
        // asBEHAVE_ENUMREFS
        .enum_refs(fp<&gc_type::enum_refs>)
        // asBEHAVE_RELEASEREFS
        .release_refs(fp<&gc_type::release_refs>);

.. note::
   You should remember to clear the GC flag in functions for increasing and decreasing reference count.
   Otherwise it is possible that the GC incorrectly determine that the object should be destroyed.

   .. code-block:: c++

    void gc_type::addref()
    {
        m_gc_flag = false;
        ++m_refcount;
    }

    void gc_type::release()
    {
        m_gc_flag = false;
        if(--m_refcount == 0)
            delete this;
    }

`You can also read the official document about the GC behaviors <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_gc_object.html#doc_reg_gcref_1>`_.

Garbage Collected Value Types
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Value types are normally not thought of as being part of circular references as they themselves cannot be referenced.
However, if a value type can hold a reference to a type, and then that type can have the value type as a member,
a circular reference can be established preventing the reference type from being released.

You only need behaviors of ``asBEHAVE_ENUMREFS`` and ``asBEHAVE_RELEASEREFS`` for a value type.

Here is an example from the ``script_optional`` of asbind20 extension library.

.. code-block:: c++

    // ...
        .enum_refs(&script_optional::enum_refs)
        .release_refs(&script_optional::release_refs)

Notifying GC of New Object
--------------------------

The GC of AngelScript needs to know that a new object is created,
this can be done automatically by ``policies::notify_gc`` of asbind20.

.. code-block:: c++

    using namespace asbind20;
    ref_class<gc_type>(/* ... */)
        .default_factory(use_policy<policies::notify_gc>)
        .factory<int>("int", use_explicit, use_policy<policies::notify_gc>)
        .factory<int, int>("int,int", use_policy<policies::notify_gc>)
        // If you want to use the default initialization list policy,
        // you can use "use_policy<void, policies::notify_gc>"
        .list_factory(
            "repeat T", use_policy<policies::repeat_list_proxy, policies::notify_gc>
        );

If you need the ``asITypeInfo*`` in a factory function to notify GC,
you can use the (list) factory function with auxiliary object.

.. code-block:: c++

    gc_type* from_init_list(void* list_buf, asITypeInfo* ti)
    {
        gc_type* p = new gc_type(/* ... */);

        ti->GetEngine()->NotifyGarbageCollectorOfNewObject(p, ti);
        return p;
    }

.. code-block:: c++

    using namespace asbind20;
    ref_class<gc_type>(/* ... */)
        .list_factory_function("repeat int", &from_init_list, auxiliary(this_type));

.. note::
    If you create objects of garbage collected type from the application side,
    you must also notify the garbage collector of its existence,
    so it's a good idea to make sure all code use the same way of creating objects of this type.
