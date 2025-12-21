#include <asbind_test/framework.hpp>
#include <sstream>
#include <asbind20/debugging/dump_bytecode.hpp>

TEST(DumpByteCode, Print)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    auto* m = engine->GetModule(
        "test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
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
    std::stringstream ss;
    debugging::print_bytecode(ss, bcs);

    EXPECT_EQ(
        ss.str(),
        "SUSPEND\n"
        "SetV4    v1, 0x2a (i:42, f:5.9e-44)\n"
        "CpyVtoR4 v1\n"
        "RET      0\n"
    );

#endif
}
