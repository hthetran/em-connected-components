project(em-cc)
cmake_minimum_required(VERSION 2.8)

option(ZERO_NODE_ASSERT "Carry out assertion that node in stream have id > 0; independent of NDEBUG flag" OFF)

# disallow in-source builds
if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
  message(SEND_ERROR "In-source builds are not allowed.")
endif("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")

# enable warnings (always good)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=100000")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

# enable CXX STANDARD
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# use nice output dirs
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# add STXXL
add_subdirectory(extlibs/tlx)
add_subdirectory(extlibs/foxxll)
add_subdirectory(extlibs/stxxl)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STXXL_CXX_FLAGS}")

# add STXXL includes path
include_directories(SYSTEM ${STXXL_INCLUDE_DIRS})

# disabling buggy parallel pq in stxxl
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL=0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL=0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTXXL_PARALLEL_PQ_MULTIWAY_MERGE_DELETE_BUFFER=0")

if (ZERO_NODE_ASSERT)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DZERO_NODE_ASSERT")
endif()

# tests
enable_testing()
add_subdirectory(extlibs/googletest/)
add_subdirectory(test)
# build some executables

# generating some graphs
add_executable(merge cpp/merge.cpp)
target_link_libraries(merge ${STXXL_LIBRARIES})

add_executable(degree_distr cpp/degree_distr.cpp)
target_link_libraries(degree_distr ${STXXL_LIBRARIES})

add_executable(remove-self-loops cpp/remove-self-loops.cpp)
target_link_libraries(remove-self-loops ${STXXL_LIBRARIES})

add_executable(gen-er cpp/gen-er.cpp)
target_link_libraries(gen-er ${STXXL_LIBRARIES})

add_executable(gen-path cpp/gen-path.cpp)
target_link_libraries(gen-path ${STXXL_LIBRARIES})

add_executable(gen-grid cpp/gen-grid.cpp)
target_link_libraries(gen-grid ${STXXL_LIBRARIES})

add_executable(gen-cube cpp/gen-cube.cpp)
target_link_libraries(gen-cube ${STXXL_LIBRARIES})

add_executable(gen-cliques cpp/gen-cliques.cpp)
target_link_libraries(gen-cliques ${STXXL_LIBRARIES})

# tools
add_executable(ascii2binary cpp/ascii2binary.cpp)
target_link_libraries(ascii2binary ${STXXL_LIBRARIES})

add_executable(binary2ascii cpp/binary2ascii.cpp)
target_link_libraries(binary2ascii ${STXXL_LIBRARIES})

add_executable(check-stars cpp/check-stars.cpp)

add_executable(compare-two-star-files cpp/compare-two-star-files.cpp)
target_link_libraries(compare-two-star-files ${STXXL_LIBRARIES})

add_executable(count-nodes cpp/count-nodes.cpp)
target_link_libraries(count-nodes ${STXXL_LIBRARIES})

add_executable(max-node-id cpp/max-node-id.cpp)
target_link_libraries(max-node-id ${STXXL_LIBRARIES})

add_executable(preprocess cpp/preprocess.cpp)
target_link_libraries(preprocess ${STXXL_LIBRARIES})

add_executable(preprocess-ram cpp/preprocess-ram.cpp)
target_link_libraries(preprocess-ram ${STXXL_LIBRARIES})

# algorithms
add_executable(run-boruvka cpp/run-boruvka.cpp)
target_link_libraries(run-boruvka ${STXXL_LIBRARIES})

add_executable(run-kruskal cpp/run-kruskal.cpp)
target_link_libraries(run-kruskal ${STXXL_LIBRARIES})

add_executable(run-sibeyn cpp/run-sibeyn.cpp)
target_link_libraries(run-sibeyn ${STXXL_LIBRARIES})

add_executable(run-sibeyn-bundles cpp/run-sibeyn-bundles.cpp)
target_link_libraries(run-sibeyn-bundles ${STXXL_LIBRARIES})

add_executable(run-streamsibeyn cpp/run-streamsibeyn.cpp)
target_link_libraries(run-streamsibeyn ${STXXL_LIBRARIES})

add_executable(run-fun-sibeyn cpp/run-fun-sibeyn.cpp)
target_link_libraries(run-fun-sibeyn ${STXXL_LIBRARIES})

#add_executable(run-fun-star cpp/run-fun-star.cpp)
#target_link_libraries(run-fun-star ${STXXL_LIBRARIES})

#add_executable(run-fun-kkt cpp/run-fun-kkt.cpp)
#target_link_libraries(run-fun-kkt ${STXXL_LIBRARIES})

#add_executable(run-vectorfun cpp/run-vectorfun.cpp)
#target_link_libraries(run-vectorfun ${STXXL_LIBRARIES})

# timing some things
add_executable(time-pq cpp/time-pq.cpp)
target_link_libraries(time-pq ${STXXL_LIBRARIES})

add_executable(time-sort cpp/time-sort.cpp)
target_link_libraries(time-sort ${STXXL_LIBRARIES})

add_executable(time-streamsort cpp/time-streamsort.cpp)
target_link_libraries(time-streamsort ${STXXL_LIBRARIES})

add_executable(time-read cpp/time-read.cpp)
target_link_libraries(time-read ${STXXL_LIBRARIES})

add_executable(time-permute cpp/time-permute.cpp)
target_link_libraries(time-permute ${STXXL_LIBRARIES})

add_executable(measure-sibeyn cpp/measure-sibeyn.cpp)
target_link_libraries(measure-sibeyn ${STXXL_LIBRARIES})

add_executable(degrees cpp/degrees.cpp)

add_executable(bench-scan cpp/bench-scan.cpp)
target_link_libraries(bench-scan ${STXXL_LIBRARIES})

add_executable(count-distinct-minsketch-computebound cpp/count-distinct-minsketch-computebound)
target_link_libraries(count-distinct-minsketch-computebound ${STXXL_LIBRARIES})
