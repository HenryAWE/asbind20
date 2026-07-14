#include <asbind_test/framework.hpp>
#include <gmock/gmock-matchers.h>
#include <sstream>
#include <asbind20/debugging/dump_bytecode.hpp>

TEST(DumpByteCode, Print)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    auto* m = asbind20::create_module(engine, "test");
    m->AddScriptSection(
        "test",
        "int f() { return 42; }"
    );
    ASSERT_GE(m->Build(), 0);
    auto* f = m->GetFunctionByName("f");
    ASSERT_NE(f, nullptr);

    auto bcs = debugging::get_bytecode(f);
    EXPECT_FALSE(bcs.empty());

#ifdef ASBIND20_HAS_LIB_FORMAT
    std::ostringstream ss;
    debugging::print_bytecode(ss, bcs);

    EXPECT_THAT(
        ss.str(),
        ::testing::HasSubstr("42")
    );

#endif
}
