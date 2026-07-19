#include <asbind_test/framework.hpp>
#include <asbind20/container/small_vector.hpp>

#ifndef ASBIND20_NO_EXCEPTIONS

TEST(SmallVector, ExceptionSafety)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    asbind_test::register_instantly_throw<true>(engine);
    asbind_test::register_throw_on_copy<true>(engine);

    // Use std::allocator for debugging
    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    {
        SCOPED_TRACE("instantly thorw");

        auto* ti = engine->GetTypeInfoByDecl("instantly_throw");
        ASSERT_NE(ti, nullptr);

        sv_type sv(ti);

        sv.emplace_back();
        EXPECT_EQ(sv.size(), 0);

        sv.emplace_back_n(2);
        EXPECT_EQ(sv.size(), 0);
    }

    {
        SCOPED_TRACE("throw on copy");

        auto* ti = engine->GetTypeInfoByDecl("throw_on_copy");
        ASSERT_NE(ti, nullptr);

        sv_type sv(ti);

        sv.emplace_back_n(2);
        EXPECT_EQ(sv.size(), 2);

        asbind_test::throw_on_copy val;
        sv.push_back_n(2, &val);
        EXPECT_EQ(sv.size(), 2);

        sv.push_back(&val);
        EXPECT_EQ(sv.size(), 2);

        sv.insert(0, &val);
        EXPECT_EQ(sv.size(), 2);
    }
}

#endif
