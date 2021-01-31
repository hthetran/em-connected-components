enable_testing()

# retrieve tests
file(GLOB TEST_SOURCES Test*.cpp)
add_executable(extmemcc_tests "${TEST_SOURCES}")

# include external libraries
#include_directories(${STXXL_INCLUDE_DIRS})
#include_directories(${gtest_SOURCE_DIR} ${gtest_SOURCE_DIR}/include)
#include_directories(${gmock_SOURCE_DIR} ${gmock_SOURCE_DIR}/include)

# link to executable
target_link_libraries(extmemcc_tests gtest gtest_main gmock gmock_main ${STXXL_LIBRARIES})
add_test(ExternalMemoryConnectedComponentsTests ../bin/extmemcc_tests)
