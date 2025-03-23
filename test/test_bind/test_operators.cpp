#include <shared_test_lib.hpp>
#include <asbind20/operators.hpp>

namespace test_bind
{
class my_pair2i
{
public:
    my_pair2i& operator+=(int val)
    {
        first += val;
        second += val;
        return *this;
    }

    friend int operator+(const my_pair2i& lhs, int val)
    {
        my_pair2i tmp = lhs;
        tmp += val;
        return tmp.first + tmp.second;
    }

    friend int operator*(const my_pair2i& lhs, const my_pair2i& rhs)
    {
        return lhs.first * rhs.first + lhs.second * rhs.second;
    }

    int first;
    int second;
};
} // namespace test_bind

TEST(test_operators, declaration)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    value_class<test_bind::my_pair2i>(engine, "pair2i")
        .behaviours_by_traits()
        .use((const_this + param<int>)->return_<int>());
}
