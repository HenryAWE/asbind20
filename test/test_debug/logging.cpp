#include <asbind_test/framework.hpp>
#include <asbind20/debugging/logging.hpp>

namespace
{
class [[maybe_unused]] recorder
{
public:
    std::array<std::string, 3> buffer;

    void log(const AS_NAMESPACE_QUALIFIER asSMessageInfo* msg)
    {
        EXPECT_STREQ(msg->section, "test");

        std::string& buf = buffer.at(msg->type);
        buf += msg->message;
        buf += '\n';
    }

    void setup(asbind20::engine_reference engine)
    {
        asbind20::set_message_callback(
            engine, &recorder::log, asbind20::auxiliary(this)
        );
    }
};
} // namespace

#ifdef ASBIND20_HAS_FMTLIB

TEST(Logging, FormattingByLevel)
{
    auto engine = asbind20::make_script_engine();
    recorder rec;
    rec.setup(*engine);

    using namespace asbind20::debugging::logging;

    const asbind20::debugging::script_source_location loc{"test"};

    info(engine, loc, "Info: {}", 1013);
    EXPECT_THAT(
        rec.buffer.at(AS_NAMESPACE_QUALIFIER asMSGTYPE_INFORMATION),
        ::testing::HasSubstr("Info: 1013")
    );

    warn(engine, loc, "Warning: {}", 42);
    EXPECT_THAT(
        rec.buffer.at(AS_NAMESPACE_QUALIFIER asMSGTYPE_WARNING),
        ::testing::HasSubstr("Warning: 42")
    );

    error(engine, loc, "Error: {}", 7);
    EXPECT_THAT(
        rec.buffer.at(AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR),
        ::testing::HasSubstr("Error: 7")
    );

    engine->ClearMessageCallback();
}

TEST(Logging, Log)
{
    auto engine = asbind20::make_script_engine();
    recorder rec;
    rec.setup(*engine);

    using namespace asbind20::debugging::logging;

    const asbind20::debugging::script_source_location loc{"test"};

    log(AS_NAMESPACE_QUALIFIER asMSGTYPE_INFORMATION, engine, loc, "Info: {}", 1013);
    EXPECT_THAT(
        rec.buffer.at(AS_NAMESPACE_QUALIFIER asMSGTYPE_INFORMATION),
        ::testing::HasSubstr("Info: 1013")
    );

    const int arg = 7;
    vlog(
        AS_NAMESPACE_QUALIFIER asMSGTYPE_WARNING,
        engine,
        loc,
        "Warn: {}",
        asbind20::io::fmtlib::make_format_args(arg)
    );
    EXPECT_THAT(
        rec.buffer.at(AS_NAMESPACE_QUALIFIER asMSGTYPE_WARNING),
        ::testing::HasSubstr("Warn: 7")
    );

    engine->ClearMessageCallback();
}

// A null engine pointer must be rejected with asINVALID_ARG by every entry point
// instead of being dereferenced.
TEST(Logging, NullEnginePointer)
{
    using namespace asbind20::debugging::logging;

    constexpr int expected = AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    const asbind20::debugging::script_source_location loc{"test"};
    asbind20::engine_pointer null_engine = nullptr;

    EXPECT_EQ(
        log(AS_NAMESPACE_QUALIFIER asMSGTYPE_INFORMATION, null_engine, loc, "log"),
        expected
    );
    EXPECT_EQ(info(null_engine, loc, "info"), expected);
    EXPECT_EQ(warn(null_engine, loc, "warn"), expected);
    EXPECT_EQ(error(null_engine, loc, "error"), expected);

    const int arg = 1;
    EXPECT_EQ(
        vlog(
            AS_NAMESPACE_QUALIFIER asMSGTYPE_WARNING,
            null_engine,
            loc,
            "warn: {}",
            asbind20::io::fmtlib::make_format_args(arg)
        ),
        expected
    );
}

#endif
