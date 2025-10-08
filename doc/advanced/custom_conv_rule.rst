Customize Type Conversion Rules
===============================

Sometimes the default auto-conversion tools provided by asbind20 is not enough for handling special user-defined types,
it requires user to provide rules for translating them between C++ and AngelScript.

Example for ``std::byte``
-------------------------

The ``std::byte`` is defined using ``enum class byte : unsigned char {};``.
However, the default logic of type translator will convert enumerations to ``int`` because all underlying type of ``enum`` in AngelScript is ``int`` by default.

.. _custom-rule-for-enum-underlying:

For enumerations with custom underlying types,
asbind20 provides an utility for quickly implementing the traits.

.. doxygenstruct:: asbind20::underlying_enum_traits

Example usage:

.. code-block:: c++

    // In the global namespace:
    template <>
    struct asbind20::type_traits<std::byte> :
        public asbind20::underlying_enum_traits<std::byte>
    {};

Here is detailed example for how to make ``std::byte`` be treated as ``uint8`` by asbind20.
You can apply this logic to the type that you want to have customized rules.

.. code-block:: c++

    // In the global namespace:
    template <>
    struct asbind20::type_traits<std::byte>
    {
        static int set_arg(asIScriptContext* ctx, asUINT arg, std::byte val)
        {
            return ctx->SetArgByte(arg, static_cast<asBYTE>(val));
        }

        static std::byte get_return(asIScriptContext* ctx)
        {
            return static_cast<std::byte>(ctx->GetReturnByte());
        }

        // If you only use native calling convention,
        // you can omit the following two functions.

        static std::byte get_arg(asIScriptGeneric* gen, asUINT arg)
        {
            return static_cast<std::byte>(gen->GetArgByte(arg));
        }

        static int set_return(asIScriptGeneric* gen, std::byte val)
        {
            return gen->SetReturnByte(static_cast<asBYTE>(val));
        }
    };

The above conversion rule is already defined in ``asbind20/type_traits.hpp``.

Reference of Conversion Rules Provided by asbind20
--------------------------------------------------

.. doxygenstruct:: asbind20::type_traits< std::byte >
.. doxygenstruct:: asbind20::type_traits< script_object >
