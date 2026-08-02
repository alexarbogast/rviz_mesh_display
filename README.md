# rviz_mesh_display

[![license - apache 2.0](https://img.shields.io/:license-Apache%202.0-yellowgreen.svg)](https://opensource.org/licenses/Apache-2.0)
[![ROS2 CI](https://github.com/alexarbogast/rviz_mesh_display/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/alexarbogast/rviz_mesh_display/actions/workflows/build-and-test.yml)

The `rviz_mesh_display` package provides an RViz2 display plugin for loading an arbitrary mesh file (STL/DAE/OBJ) from disk and attaching it to a TF frame.
This facilitates viewing temporary, standalone models without publishing a `visualization_msgs/Marker` or adding a temporary URDF link.

<p align="center">
    <img src="docs/images/screenshot.png" width="600"/>
</p>

The display also provides options for modifying the appearance and transformation of the model.
