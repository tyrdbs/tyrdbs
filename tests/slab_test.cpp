#include <common/slabs.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>


using namespace tyrtech;


TEST_CASE("test1")
{
    slabs<int> s;

    s.extend(std::make_unique<slabs<int>::slab>(5));

    {
        CHECK(s.allocate() == 0);
        s.init(0) = 1;
    }
}

TEST_CASE("test2")
{
    slabs<int>::slab s(5);

    for (uint16_t i = 0; i < s.capacity(); i++)
    {
        s.init(i) = i;
    }

    CHECK(s.full() == false);
    CHECK(s.allocate() == 0);
    CHECK(s.allocate() == 1);
    CHECK(s.allocate() == 2);
    CHECK(s.allocate() == 3);
    CHECK(s.allocate() == 4);
    CHECK(s.full() == true);

    s.free(3);
    CHECK(s.allocate() == 3);
    CHECK(s.full() == true);

    s.free(3);
    s.free(4);
    s.free(0);
    s.free(1);
    s.free(2);
    CHECK(s.full() == false);

    CHECK(s.allocate() == 2);
    CHECK(s.allocate() == 1);
    CHECK(s.allocate() == 0);
    CHECK(s.allocate() == 4);
    CHECK(s.allocate() == 3);
    CHECK(s.full() == true);
}

TEST_CASE("test3")
{
    slabs<int> s;

    s.extend(std::make_unique<slabs<int>::slab>(1));
    s.extend(std::make_unique<slabs<int>::slab>(1));
    s.extend(std::make_unique<slabs<int>::slab>(1));
    s.extend(std::make_unique<slabs<int>::slab>(1));
    s.extend(std::make_unique<slabs<int>::slab>(1));

    CHECK(s.full() == false);
    CHECK(s.allocate() == 0x00000000U);
    CHECK(s.allocate() == 0x00010000U);
    CHECK(s.allocate() == 0x00020000U);
    CHECK(s.allocate() == 0x00030000U);
    CHECK(s.allocate() == 0x00040000U);
    CHECK(s.full() == true);

    s.free(0x00030000U);
    CHECK(s.allocate() == 0x00030000U);
    CHECK(s.full() == true);

    s.free(0x00030000U);
    s.free(0x00040000U);
    s.free(0x00000000U);
    s.free(0x00010000U);
    s.free(0x00020000U);
    CHECK(s.full() == false);

    CHECK(s.allocate() == 0x00030000U);
    CHECK(s.allocate() == 0x00040000U);
    CHECK(s.allocate() == 0x00000000U);
    CHECK(s.allocate() == 0x00010000U);
    CHECK(s.allocate() == 0x00020000U);
    CHECK(s.full() == true);
}
