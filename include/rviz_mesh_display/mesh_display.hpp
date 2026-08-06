// Copyright 2026 Alex Arbogast
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RVIZ_MESH_DISPLAY__MESH_DISPLAY_HPP_
#define RVIZ_MESH_DISPLAY__MESH_DISPLAY_HPP_

#include <set>
#include <string>

#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <OgreMaterial.h>

#include <rviz_common/display.hpp>

namespace Ogre
{
class Entity;
class SceneNode;
}  // namespace Ogre

namespace rviz_common::properties
{
class FilePickerProperty;
class TfFrameProperty;
class VectorProperty;
class BoolProperty;
class ColorProperty;
class FloatProperty;
}  // namespace rviz_common::properties

namespace rviz_mesh_display
{

/**
 * @brief RViz display that loads a static mesh file (STL/DAE/OBJ) from disk
 * and renders it attached to a chosen TF frame with a fixed offset.
 *
 * Unlike rviz_default_plugins::RobotModelDisplay or the Marker/MarkerArray
 * displays, this requires no URDF, no publishing node, and no topic --
 * just a file path and a frame selection.
 */
class MeshDisplay : public rviz_common::Display
{
  Q_OBJECT

public:
  MeshDisplay();
  ~MeshDisplay() override;

  void onInitialize() override;
  void reset() override;
  void update(float wall_dt, float ros_dt) override;

protected:
  void onEnable() override;
  void onDisable() override;

private Q_SLOTS:
  void updateMesh();
  void updateColorAndAlpha();

private:
  void loadMesh();
  void clearMesh();
  void applyMaterials();
  void updateTransform();

  rviz_common::properties::FilePickerProperty* mesh_file_property_;
  rviz_common::properties::TfFrameProperty* frame_property_;
  rviz_common::properties::VectorProperty* position_offset_property_;
  rviz_common::properties::VectorProperty* rpy_offset_property_;
  rviz_common::properties::VectorProperty* scale_property_;
  rviz_common::properties::BoolProperty* use_embedded_material_property_;
  rviz_common::properties::ColorProperty* color_property_;
  rviz_common::properties::FloatProperty* alpha_property_;

  Ogre::Entity* entity_{ nullptr };

  std::set<Ogre::MaterialPtr> materials_;
  Ogre::MaterialPtr default_material_;

  std::string loaded_resource_;
  uint32_t instance_id_;
};

}  // namespace rviz_mesh_display

#endif  // RVIZ_MESH_DISPLAY__MESH_DISPLAY_HPP_
