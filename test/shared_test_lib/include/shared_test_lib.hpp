#pragma once

#include <filesystem>
#include <gtest/gtest.h>
#include <asbind20/asbind.hpp>

namespace asbind_test
{
template <typename T>
::testing::AssertionResult result_has_value(const asbind20::script_invoke_result<T>& r)
{
    if(r.has_value())
        return ::testing::AssertionSuccess();
    else
    {
        const char* state_str = "";
        switch(r.error())
        {
        case AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED: state_str = "FINISHED"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_SUSPENDED: state_str = "SUSPENDED"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_ABORTED: state_str = "ABORTED"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION: state_str = "EXCEPTION"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_PREPARED: state_str = "PREPARED"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_UNINITIALIZED: state_str = "UNINITIALIZED"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_ACTIVE: state_str = "ACTIVE"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_ERROR: state_str = "ERROR"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_DESERIALIZATION: state_str = "DESERIALIZATION"; break;
        }

        return ::testing::AssertionFailure()
               << "r = " << r.error() << ' ' << state_str;
    }
}

void setup_message_callback(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool propagate_error_to_gtest = false
);

void setup_exception_translator(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
);

void output_gc_statistics(
    std::ostream& os,
    const asbind20::debugging::gc_statistics& stat,
    char sep = '\n'
);
void output_gc_statistics(
    std::ostream& os,
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    char sep = '\n'
);

class expected_ex : public std::exception
{
public:
    static constexpr char info[] = "expected exception";

    const char* what() const noexcept override
    {
        return info;
    }
};

class instantly_throw
{
public:
    instantly_throw()
        : m_placeholder{}
    {
        throw expected_ex{};
    }

    instantly_throw(const instantly_throw&) = default;

    ~instantly_throw() = default;

private:
    [[maybe_unused]]
    int m_placeholder[4];
};

class throw_on_copy
{
public:
    throw_on_copy()
        : m_placeholder{} {}

    throw_on_copy(const throw_on_copy&)
    {
        throw expected_ex{};
    }

    ~throw_on_copy() = default;

    throw_on_copy& operator=(const throw_on_copy&)
    {
        throw expected_ex{};
    }

private:
    [[maybe_unused]]
    int m_placeholder[4];
};

template <bool UseGeneric>
void register_instantly_throw(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    AS_NAMESPACE_QUALIFIER asQWORD flags =
        AS_NAMESPACE_QUALIFIER asGetTypeTraits<instantly_throw>() |
        AS_NAMESPACE_QUALIFIER asOBJ_POD |
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS;

    value_class<instantly_throw, UseGeneric>(
        engine, "instantly_throw", flags
    )
        .behaviours_by_traits(flags);
}

template <bool UseGeneric>
void register_throw_on_copy(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    AS_NAMESPACE_QUALIFIER asQWORD flags =
        AS_NAMESPACE_QUALIFIER asGetTypeTraits<throw_on_copy>() |
        AS_NAMESPACE_QUALIFIER asOBJ_POD |
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS;

    value_class<throw_on_copy, UseGeneric>(
        engine, "throw_on_copy", flags
    )
        .behaviours_by_traits(flags);
}

class asbind_test_suite : public ::testing::Test
{
public:
    void msg_callback(const AS_NAMESPACE_QUALIFIER asSMessageInfo* msg);
    void ex_translator(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx);

    void SetUp() override;

    void TearDown() override;

    AS_NAMESPACE_QUALIFIER asIScriptEngine* get_engine() const noexcept
    {
        return m_engine.get();
    }

    void run_file(
        const std::filesystem::path& filename,
        const char* entry_decl = "void main()"
    );

protected:
    virtual void register_all();

private:
    asbind20::script_engine m_engine;
};

class asbind_test_suite_generic : public asbind_test_suite
{
protected:
    void register_all() override;
};
} // namespace asbind_test
