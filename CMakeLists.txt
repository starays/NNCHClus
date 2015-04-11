cmake_minimum_required(VERSION 3.1)
project(NNCHClus)

include_directories(src/public)

add_library(aggl_hier_clusterer src/internal/aggl_hier_clusterer.cc)

add_executable(aggl_test src/public/aggl_hirecluster_test.cc)
target_link_libraries(aggl_test aggl_hier_clusterer)