#include <common/allocator.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>


using namespace tyrtech;


TEST_CASE("test1")
{
    allocator a;

    a.extend(5);
    CHECK(a.capacity() == 5);

    SUBCASE("sub1")
    {
        CHECK(a.allocate(5) == 0);
        CHECK(a.size() == 5);

        a.free(0, 1);
        CHECK(a.size() == 4);

        a.free(4, 1);
        CHECK(a.size() == 3);

        a.free(1, 1);
        CHECK(a.size() == 2);

        a.free(3, 1);
        CHECK(a.size() == 1);

        a.free(2, 1);
        CHECK(a.size() == 0);

        CHECK(a.allocate(5) == 0);
    }

    SUBCASE("sub2")
    {
        CHECK(a.allocate(5) == 0);
        CHECK(a.size() == 5);

        a.free(1, 3);
        CHECK(a.size() == 2);

        a.free(0, 1);
        CHECK(a.size() == 1);

        a.free(4, 1);
        CHECK(a.size() == 0);

        CHECK(a.allocate(5) == 0);
    }

    SUBCASE("sub3")
    {
        CHECK(a.allocate(10) == static_cast<uint32_t>(-1));
        CHECK(a.size() == 0);

        a.extend(5);
        CHECK(a.capacity() == 10);

        CHECK(a.allocate(10) == 0);
        CHECK(a.size() == 10);
    }
}
