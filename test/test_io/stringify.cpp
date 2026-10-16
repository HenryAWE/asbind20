#include <asbind20/asbind.hpp>
#include <asbind20/io/formatter.hpp>
#include <asbind_test/framework.hpp>

TEST(Stringify, asEContextStateToString)
{
    using namespace asbind20;

    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED), "asEXECUTION_FINISHED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_SUSPENDED), "asEXECUTION_SUSPENDED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_ABORTED), "asEXECUTION_ABORTED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION), "asEXECUTION_EXCEPTION");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_PREPARED), "asEXECUTION_PREPARED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_UNINITIALIZED), "asEXECUTION_UNINITIALIZED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_ACTIVE), "asEXECUTION_ACTIVE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_ERROR), "asEXECUTION_ERROR");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_DESERIALIZATION), "asEXECUTION_DESERIALIZATION");

    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEContextState(-1)), "asEContextState(-1)");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEContextState(-2)), "asEContextState(-2)");

    EXPECT_EQ(to_wstring(AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED), L"asEXECUTION_FINISHED");
    EXPECT_EQ(to_wstring(AS_NAMESPACE_QUALIFIER asEContextState(-1)), L"asEContextState(-1)");
}

TEST(Stringify, asERetCodesToString)
{
    using namespace asbind20;

    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asSUCCESS), "asSUCCESS");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asERROR), "asERROR");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asCONTEXT_ACTIVE), "asCONTEXT_ACTIVE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asCONTEXT_NOT_FINISHED), "asCONTEXT_NOT_FINISHED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asCONTEXT_NOT_PREPARED), "asCONTEXT_NOT_PREPARED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_ARG), "asINVALID_ARG");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asNO_FUNCTION), "asNO_FUNCTION");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asNOT_SUPPORTED), "asNOT_SUPPORTED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_NAME), "asINVALID_NAME");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asNAME_TAKEN), "asNAME_TAKEN");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_DECLARATION), "asINVALID_DECLARATION");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_OBJECT), "asINVALID_OBJECT");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_TYPE), "asINVALID_TYPE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asALREADY_REGISTERED), "asALREADY_REGISTERED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asMULTIPLE_FUNCTIONS), "asMULTIPLE_FUNCTIONS");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asNO_MODULE), "asNO_MODULE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asNO_GLOBAL_VAR), "asNO_GLOBAL_VAR");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_CONFIGURATION), "asINVALID_CONFIGURATION");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_INTERFACE), "asINVALID_INTERFACE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asCANT_BIND_ALL_FUNCTIONS), "asCANT_BIND_ALL_FUNCTIONS");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asLOWER_ARRAY_DIMENSION_NOT_REGISTERED), "asLOWER_ARRAY_DIMENSION_NOT_REGISTERED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asWRONG_CONFIG_GROUP), "asWRONG_CONFIG_GROUP");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asCONFIG_GROUP_IS_IN_USE), "asCONFIG_GROUP_IS_IN_USE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asILLEGAL_BEHAVIOUR_FOR_TYPE), "asILLEGAL_BEHAVIOUR_FOR_TYPE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asWRONG_CALLING_CONV), "asWRONG_CALLING_CONV");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asBUILD_IN_PROGRESS), "asBUILD_IN_PROGRESS");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINIT_GLOBAL_VARS_FAILED), "asINIT_GLOBAL_VARS_FAILED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asOUT_OF_MEMORY), "asOUT_OF_MEMORY");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asMODULE_IS_IN_USE), "asMODULE_IS_IN_USE");

    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asERetCodes(1)), "asERetCodes(1)");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asERetCodes(2)), "asERetCodes(2)");

    EXPECT_EQ(to_wstring(AS_NAMESPACE_QUALIFIER asSUCCESS), L"asSUCCESS");
    EXPECT_EQ(to_wstring(AS_NAMESPACE_QUALIFIER asERetCodes(1)), L"asERetCodes(1)");
}

TEST(Stringify, asEMsgTypeToString)
{
    using namespace asbind20;

    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR), "asMSGTYPE_ERROR");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asMSGTYPE_WARNING), "asMSGTYPE_WARNING");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asMSGTYPE_INFORMATION), "asMSGTYPE_INFORMATION");

    EXPECT_EQ(to_wstring(AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR), L"asMSGTYPE_ERROR");
}

TEST(Stringify, asETokenClass)
{
    using namespace asbind20;

    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asTC_UNKNOWN), "asTC_UNKNOWN");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asTC_KEYWORD), "asTC_KEYWORD");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asTC_VALUE), "asTC_VALUE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER), "asTC_IDENTIFIER");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asTC_COMMENT), "asTC_COMMENT");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asTC_WHITESPACE), "asTC_WHITESPACE");
}

TEST(Stringify, asEEnginePropToString)
{
    using namespace asbind20;

    EXPECT_EQ(
        to_string(AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES),
        "asEP_ALLOW_UNSAFE_REFERENCES"
    );
    EXPECT_EQ(
        to_string(AS_NAMESPACE_QUALIFIER asEEngineProp(-1)),
        "asEEngineProp(-1)"
    );

    EXPECT_EQ(
        to_wstring(AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES),
        L"asEP_ALLOW_UNSAFE_REFERENCES"
    );
    EXPECT_EQ(
        to_wstring(AS_NAMESPACE_QUALIFIER asEEngineProp(-1)),
        L"asEEngineProp(-1)"
    );
}

TEST(Stringify, CopyDebugRepresentation)
{
    namespace io = asbind20::io;

    {
        std::string s;
        io::copy_debug_representation_to(
            AS_NAMESPACE_QUALIFIER asALREADY_REGISTERED,
            true,
            std::back_inserter(s)
        );
        EXPECT_EQ(s, "ALREADY_REGISTERED");
    }

    {
        std::string s;
        io::copy_debug_representation_to(
            AS_NAMESPACE_QUALIFIER asALREADY_REGISTERED,
            false,
            std::back_inserter(s)
        );
        EXPECT_EQ(s, "asALREADY_REGISTERED");
    }

    {
        std::string s;
        io::copy_debug_representation_to(
            AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES,
            true,
            std::back_inserter(s)
        );
        EXPECT_EQ(s, "ALLOW_UNSAFE_REFERENCES");
    }

    {
        std::string s;
        io::copy_debug_representation_to(
            AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES,
            false,
            std::back_inserter(s)
        );
        EXPECT_EQ(s, "asEP_ALLOW_UNSAFE_REFERENCES");
    }

    {
        std::string s;
        io::copy_debug_representation_to(
            AS_NAMESPACE_QUALIFIER asEEngineProp(-1),
            false,
            std::back_inserter(s)
        );
        EXPECT_EQ(s, "asEEngineProp(-1)");
    }
}

#ifdef ASBIND20_HAS_FORMAT_LIBRARY

TEST(Stringify, Formatter)
{
    using namespace asbind20::io::fmtlib;

    EXPECT_EQ(
        format("{}", AS_NAMESPACE_QUALIFIER asALREADY_REGISTERED),
        "ALREADY_REGISTERED"
    );
    EXPECT_EQ(
        format("{:?}", AS_NAMESPACE_QUALIFIER asALREADY_REGISTERED),
        "asALREADY_REGISTERED"
    );
    EXPECT_EQ(
        format(L"{:?}", AS_NAMESPACE_QUALIFIER asALREADY_REGISTERED),
        L"asALREADY_REGISTERED"
    );
    EXPECT_EQ(
        format("{}", AS_NAMESPACE_QUALIFIER asERetCodes(1)),
        "asERetCodes(1)"
    );
    EXPECT_EQ(
        format("{:d}", AS_NAMESPACE_QUALIFIER asERetCodes(1)),
        "1"
    );
    EXPECT_EQ(
        format("{:x}", AS_NAMESPACE_QUALIFIER asERetCodes(15)),
        "f"
    );
    EXPECT_EQ(
        format(L"{:d}", AS_NAMESPACE_QUALIFIER asERetCodes(1)),
        L"1"
    );
    EXPECT_EQ(
        format(L"{:x}", AS_NAMESPACE_QUALIFIER asERetCodes(15)),
        L"f"
    );

    EXPECT_EQ(
        format("{}", AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED),
        "FINISHED"
    );
    EXPECT_EQ(
        format("{:?}", AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED),
        "asEXECUTION_FINISHED"
    );
    EXPECT_EQ(
        format(L"{:?}", AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED),
        L"asEXECUTION_FINISHED"
    );
    EXPECT_EQ(
        format("{}", AS_NAMESPACE_QUALIFIER asEContextState(-1)),
        "asEContextState(-1)"
    );
    EXPECT_EQ(
        format("{:d}", AS_NAMESPACE_QUALIFIER asEContextState(-1)),
        "-1"
    );
    EXPECT_EQ(
        format(L"{:d}", AS_NAMESPACE_QUALIFIER asEContextState(-1)),
        L"-1"
    );

    EXPECT_EQ(
        format("{}", AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR),
        "ERROR"
    );
    EXPECT_EQ(
        format(L"{}", AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR),
        L"ERROR"
    );

    EXPECT_EQ(
        format("{}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER),
        "IDENTIFIER"
    );
    EXPECT_EQ(
        format(L"{}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER),
        L"IDENTIFIER"
    );

    EXPECT_EQ(
        format("{}", AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES),
        "ALLOW_UNSAFE_REFERENCES"
    );
    EXPECT_EQ(
        format("{:?}", AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES),
        "asEP_ALLOW_UNSAFE_REFERENCES"
    );
    EXPECT_EQ(
        format(L"{:?}", AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES),
        L"asEP_ALLOW_UNSAFE_REFERENCES"
    );
    EXPECT_EQ(
        format("{}", AS_NAMESPACE_QUALIFIER asEEngineProp(-1)),
        "asEEngineProp(-1)"
    );
    EXPECT_EQ(
        format("{:d}", AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES),
        "1"
    );
    EXPECT_EQ(
        format(L"{:d}", AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES),
        L"1"
    );
}

TEST(Stringify, EnumFormatterCommonSpec)
{
    using namespace asbind20::io::fmtlib;

    // The underlying value is formatted exactly like an integer
    for(int value : {0, 1, -1, 15, -16, 255, 2147483647, -2147483647 - 1})
    {
        EXPECT_EQ(
            format("{:d}", AS_NAMESPACE_QUALIFIER asERetCodes(value)),
            format("{:d}", value)
        );
        EXPECT_EQ(
            format("{:x}", AS_NAMESPACE_QUALIFIER asERetCodes(value)),
            format("{:x}", value)
        );
        EXPECT_EQ(
            format("{:>12d}", AS_NAMESPACE_QUALIFIER asERetCodes(value)),
            format("{:>12d}", value)
        );
    }

    // Fill, alignment and width of the enumerator name
    EXPECT_EQ(format("{:12}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER), "IDENTIFIER  ");
    EXPECT_EQ(format("{:<12}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER), "IDENTIFIER  ");
    EXPECT_EQ(format("{:>12}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER), "  IDENTIFIER");
    EXPECT_EQ(format("{:^12}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER), " IDENTIFIER ");
    EXPECT_EQ(format("{:*>12}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER), "**IDENTIFIER");
    EXPECT_EQ(format("{:*<12}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER), "IDENTIFIER**");
    EXPECT_EQ(format("{:*^12}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER), "*IDENTIFIER*");
    // A width smaller than the field does not truncate it
    EXPECT_EQ(format("{:*>4}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER), "IDENTIFIER");

    // ... and of the full representation
    EXPECT_EQ(
        format("{:*>18?}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER),
        "***asTC_IDENTIFIER"
    );

    // The underlying value is aligned like an integer, i.e. right by default
    EXPECT_EQ(
        format("{:8d}", AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES),
        "       1"
    );
    EXPECT_EQ(
        format("{:*>8d}", AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES),
        "*******1"
    );
    EXPECT_EQ(format("{:*<8x}", AS_NAMESPACE_QUALIFIER asERetCodes(15)), "f*******");
    EXPECT_EQ(format("{:^8x}", AS_NAMESPACE_QUALIFIER asERetCodes(15)), "   f    ");

    // Wide characters are supported as well
    EXPECT_EQ(
        format(L"{:*>12}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER),
        L"**IDENTIFIER"
    );
    EXPECT_EQ(
        format(L"{:8d}", AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES),
        L"       1"
    );

    // Dynamic width
    EXPECT_EQ(
        format("{:{}}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER, 12),
        "IDENTIFIER  "
    );
    EXPECT_EQ(
        format("{:*^{}}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER, 12),
        "*IDENTIFIER*"
    );
    EXPECT_EQ(
        format("{:>{}d}", AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES, 8),
        "       1"
    );
    EXPECT_EQ(
        format("{0:>{1}d}", AS_NAMESPACE_QUALIFIER asEP_ALLOW_UNSAFE_REFERENCES, 8),
        "       1"
    );
    EXPECT_EQ(
        format(L"{:*^{}}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER, 12),
        L"*IDENTIFIER*"
    );

    // Dynamic width combined with a presentation type
    EXPECT_EQ(
        format("{:*>{}?}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER, 18),
        "***asTC_IDENTIFIER"
    );
    EXPECT_EQ(
        format("{:*<{}x}", AS_NAMESPACE_QUALIFIER asERetCodes(15), 8),
        "f*******"
    );
}

#    if !defined(ASBIND20_NO_EXCEPTIONS)

TEST(Stringify, EnumFormatterError)
{
    using namespace asbind20::io;

    EXPECT_THROW(
        (void)fmtlib::format("{:{}}", AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER, "invalid"),
        fmtlib::format_error
    );
}

#    endif

#endif
