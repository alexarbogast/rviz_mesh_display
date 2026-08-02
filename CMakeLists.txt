cmake_minimum_required(VERSION 3.8)
project(rviz_mesh_display)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rviz_common REQUIRED)
find_package(rviz_rendering REQUIRED)
find_package(rviz_ogre_vendor REQUIRED)
find_package(pluginlib REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(resource_retriever REQUIRED)
find_package(rclcpp REQUIRED)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(${PROJECT_NAME}_headers_to_moc
  include/rviz_mesh_display/mesh_display.hpp
)
foreach(header ${${PROJECT_NAME}_headers_to_moc})
  qt5_wrap_cpp(${PROJECT_NAME}_moc_files "${header}")
endforeach()

add_library(${PROJECT_NAME} SHARED
  src/mesh_display.cpp
  ${${PROJECT_NAME}_moc_files}
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
  rviz_common::rviz_common
  rviz_rendering::rviz_rendering
  Qt5::Widgets
)

ament_target_dependencies(${PROJECT_NAME} PUBLIC
  pluginlib
  tf2_ros
  resource_retriever
  rclcpp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE "RVIZ_MESH_DISPLAY_BUILDING_LIBRARY")

pluginlib_export_plugin_description_file(rviz_common plugin_description.xml)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  FILES plugin_description.xml
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})
ament_export_targets(export_${PROJECT_NAME})
ament_export_dependencies(
  rviz_common
  rviz_rendering
  pluginlib
  tf2_ros
  resource_retriever
  rclcpp
)

# if(BUILD_TESTING)
#   find_package(ament_lint_auto REQUIRED)
#   ament_lint_auto_find_test_dependencies()
# endif()

ament_package()
