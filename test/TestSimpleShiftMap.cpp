#include <gtest/gtest.h>
#include "../cpp/simpleshiftmap.hpp"

class TestSimpleShiftMap : public ::testing::Test { };

TEST_F(TestSimpleShiftMap, test_1) {
	SimpleShiftMap<int, std::string> map(10, 19);
    ASSERT_EQ(map.size(), 0ul);
    map[10] = "ten";
    ASSERT_EQ(map[10], "ten");
    map[11] = "eleven";
    ASSERT_EQ(map.size(), 2ul);
    ASSERT_EQ(map[11], "eleven");
    ASSERT_TRUE(map.contains(11));
    ASSERT_FALSE(map.contains(12));
}
