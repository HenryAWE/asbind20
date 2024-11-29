# Customize Type Conversion Rules
Sometimes the default auto-conversion tools provided by asbind20 is not enough for handling special use-defined types, it requires user to provide rules for translating them between C++ and AngelScript.

## Example for `std::byte`
The `std::byte` is defined using `enum class byte : unsigned char {};`. However, the default logic of type translator will converted enumerations to `DWord` (`int32`) because all underlying type of `enum` in AngelScript is `int`.

Here is how to make `std::byte` be treated as `uint8` by asbind20.
```c++
namespace asbind20
{
template <>
struct type_traits<std::byte>
{
    static int set_arg(asIScriptContext* ctx, asUINT arg, std::byte val)
    {
        return ctx->SetArgByte(arg, static_cast<asBYTE>(val));
    }

    static std::byte get_arg(asIScriptGeneric* gen, asUINT arg)
    {
        return static_cast<std::byte>(gen->GetArgByte(arg));
    }

    static int set_return(asIScriptGeneric* gen, std::byte val)
    {
        return gen->SetReturnByte(static_cast<asBYTE>(val));
    }

    static std::byte get_return(asIScriptContext* ctx)
    {
        return static_cast<std::byte>(ctx->GetReturnByte());
    }
};
} // namespace asbind20
```
