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

#include "rviz_mesh_display/mesh_display.hpp"

#include <string>

#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreSubEntity.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgrePass.h>

#include <rviz_common/display_context.hpp>
#include <rviz_common/frame_manager_iface.hpp>
#include <rviz_common/properties/file_picker_property.hpp>
#include <rviz_common/properties/tf_frame_property.hpp>
#include <rviz_common/properties/vector_property.hpp>
#include <rviz_common/properties/bool_property.hpp>
#include <rviz_common/properties/color_property.hpp>
#include <rviz_common/properties/float_property.hpp>
#include <rviz_common/properties/status_property.hpp>
#include <rviz_rendering/mesh_loader.hpp>
#include <rviz_rendering/material_manager.hpp>

namespace rviz_mesh_display
{

namespace
{
// Ogre requires every mesh/entity/material name to be globally unique
// within its resource groups, so every MeshDisplay instance gets its own
// numeric suffix for the resources it creates.
uint32_t nextInstanceId()
{
  static uint32_t counter = 0;
  return counter++;
}

std::string toResourceUrl(const std::string& path)
{
  if (path.empty())
  {
    return path;
  }
  if (path.find("://") != std::string::npos)
  {
    return path;
  }
  return "file://" + path;
}
}  // namespace

MeshDisplay::MeshDisplay() : Display(), instance_id_(nextInstanceId())
{
  mesh_file_property_ = new rviz_common::properties::FilePickerProperty(
      "Mesh File", "",
      "Path to a mesh file (.stl, .dae, .obj). Also accepts package:// URLs.",
      this, SLOT(updateMesh()));

  frame_property_ = new rviz_common::properties::TfFrameProperty(
      "Reference Frame",
      rviz_common::properties::TfFrameProperty::FIXED_FRAME_STRING,
      "TF frame the mesh is attached to.", this, nullptr, true);

  position_offset_property_ = new rviz_common::properties::VectorProperty(
      "Position Offset", Ogre::Vector3::ZERO,
      "Translation applied on top of the reference frame, in the frame's own "
      "axes (meters).",
      this);

  rpy_offset_property_ = new rviz_common::properties::VectorProperty(
      "Rotation Offset (RPY, deg)", Ogre::Vector3::ZERO,
      "Roll/Pitch/Yaw applied on top of the reference frame's orientation, in "
      "degrees.",
      this);

  scale_property_ = new rviz_common::properties::VectorProperty(
      "Scale", Ogre::Vector3::UNIT_SCALE,
      "Per-axis scale applied to the mesh about the model origin", this);

  use_embedded_material_property_ = new rviz_common::properties::BoolProperty(
      "Use Embedded Material", true,
      "Use the mesh file's own colors/materials if it has them. "
      "Turn off to force the Color/Alpha below onto the whole mesh.",
      this, SLOT(updateMesh()));

  color_property_ = new rviz_common::properties::ColorProperty(
      "Color", QColor(200, 200, 200),
      "Fallback color, used when Use Embedded Material is off (or the mesh has "
      "no material).",
      this, SLOT(updateColorAndAlpha()));

  alpha_property_ = new rviz_common::properties::FloatProperty(
      "Alpha", 1.0f, "Opacity: 0 is fully transparent, 1 is fully opaque.",
      this, SLOT(updateColorAndAlpha()));
  alpha_property_->setMin(0.0f);
  alpha_property_->setMax(1.0f);
}

MeshDisplay::~MeshDisplay()
{
  clearMesh();
}

void MeshDisplay::onInitialize()
{
  frame_property_->setFrameManager(context_->getFrameManager());
}

void MeshDisplay::reset()
{
  Display::reset();
  clearMesh();
  loadMesh();
}

void MeshDisplay::onEnable()
{
  if (!entity_)
  {
    loadMesh();
  }
  scene_node_->setVisible(true);
}

void MeshDisplay::onDisable()
{
  scene_node_->setVisible(false);
}

void MeshDisplay::update(float wall_dt, float ros_dt)
{
  (void)wall_dt;
  (void)ros_dt;
  updateTransform();
}

void MeshDisplay::clearMesh()
{
  if (entity_)
  {
    scene_node_->detachObject(entity_);
    scene_manager_->destroyEntity(entity_);
    entity_ = nullptr;
  }

  for (const auto& material : materials_)
  {
    if (material)
    {
      material->unload();
      Ogre::MaterialManager::getSingleton().remove(material->getName(),
                                                   material->getGroup());
    }
  }
  materials_.clear();
  default_material_.reset();

  loaded_resource_.clear();
}

void MeshDisplay::updateMesh()
{
  clearMesh();
  loadMesh();
}

void MeshDisplay::loadMesh()
{
  if (!isEnabled())
  {
    return;
  }

  const std::string path = mesh_file_property_->getStdString();
  if (path.empty())
  {
    setStatus(rviz_common::properties::StatusProperty::Warn, "Mesh",
              "No mesh file selected.");
    return;
  }

  const std::string resource = toResourceUrl(path);

  const Ogre::MeshPtr mesh = rviz_rendering::loadMeshFromResource(resource);
  if (!mesh)
  {
    setStatus(rviz_common::properties::StatusProperty::Error, "Mesh",
              QString("Failed to load [%1]. See the RViz console/log for the "
                      "underlying error.")
                  .arg(QString::fromStdString(resource)));
    return;
  }

  const std::string entity_name = "rviz_mesh_display_" +
                                  std::to_string(instance_id_) + "_" +
                                  std::to_string(nextInstanceId());
  entity_ = scene_manager_->createEntity(entity_name, resource);
  scene_node_->attachObject(entity_);

  applyMaterials();

  loaded_resource_ = resource;
  setStatus(rviz_common::properties::StatusProperty::Ok, "Mesh",
            "Mesh loaded.");

  updateTransform();
}

void MeshDisplay::applyMaterials()
{
  if (!entity_)
  {
    return;
  }

  // One fallback material per instance, used for any sub-entity that has no
  // material of its own (Ogre calls that "BaseWhiteNoLighting"), and for the
  // entire mesh when "Use Embedded Material" is off.
  const std::string material_name = "rviz_mesh_display_material_" +
                                    std::to_string(instance_id_) + "_" +
                                    std::to_string(nextInstanceId());
  default_material_ =
      rviz_rendering::MaterialManager::createMaterialWithLighting(
          material_name);
  materials_.insert(default_material_);

  if (use_embedded_material_property_->getBool())
  {
    // Clone each embedded material so tweaking alpha here never mutates a
    // material some other display/entity is also using.
    for (uint32_t i = 0; i < entity_->getNumSubEntities(); ++i)
    {
      Ogre::SubEntity* sub_entity = entity_->getSubEntity(i);
      const std::string original_name = sub_entity->getMaterialName();

      if (original_name == "BaseWhiteNoLighting")
      {
        // No embedded material on this sub-mesh
        sub_entity->setMaterial(default_material_);
        continue;
      }

      Ogre::MaterialPtr original = sub_entity->getMaterial();
      const std::string clone_name = material_name + "_" + std::to_string(i);
      Ogre::MaterialPtr clone = original->clone(clone_name);
      materials_.insert(clone);
      sub_entity->setMaterial(clone);
    }
  }
  else
  {
    entity_->setMaterial(default_material_);
  }

  updateColorAndAlpha();
}

void MeshDisplay::updateColorAndAlpha()
{
  if (!entity_)
  {
    return;
  }

  const float alpha = alpha_property_->getFloat();
  const bool use_embedded = use_embedded_material_property_->getBool();

  Ogre::SceneBlendType blending;
  bool depth_write;
  rviz_rendering::MaterialManager::enableAlphaBlending(blending, depth_write,
                                                       alpha);

  if (use_embedded)
  {
    // Leave the embedded colors alone: only adjust transparency across every
    // pass of every cloned embedded material.
    for (const auto& material : materials_)
    {
      if (!material)
      {
        continue;
      }
      for (unsigned short t = 0; t < material->getNumTechniques(); ++t)
      {
        Ogre::Technique* technique = material->getTechnique(t);
        for (unsigned short p = 0; p < technique->getNumPasses(); ++p)
        {
          Ogre::Pass* pass = technique->getPass(p);
          Ogre::ColourValue diffuse = pass->getDiffuse();
          diffuse.a = alpha;
          pass->setDiffuse(diffuse);
          pass->setSceneBlending(blending);
          pass->setDepthWriteEnabled(depth_write);
        }
      }
    }
  }
  else
  {
    // Full override: paint the whole mesh with the Color property.
    const Ogre::ColourValue color = color_property_->getOgreColor();
    Ogre::Pass* pass = default_material_->getTechnique(0)->getPass(0);
    pass->setAmbient(color.r * 0.5f, color.g * 0.5f, color.b * 0.5f);
    pass->setDiffuse(color.r, color.g, color.b, alpha);
    pass->setSceneBlending(blending);
    pass->setDepthWriteEnabled(depth_write);
    pass->setLightingEnabled(true);
  }

  context_->queueRender();
}

void MeshDisplay::updateTransform()
{
  const std::string frame = frame_property_->getFrameStd();

  Ogre::Vector3 frame_position;
  Ogre::Quaternion frame_orientation;

  if (!context_->getFrameManager()->getTransform(frame, frame_position,
                                                 frame_orientation))
  {
    std::string error;
    if (context_->getFrameManager()->transformHasProblems(
            frame, context_->getClock()->now(), error))
    {
      setStatus(rviz_common::properties::StatusProperty::Error, "Transform",
                QString::fromStdString(error));
    }
    else
    {
      setMissingTransformToFixedFrame(frame);
    }
    scene_node_->setVisible(false);
    return;
  }

  if (isEnabled())
  {
    scene_node_->setVisible(true);
  }
  setStatus(rviz_common::properties::StatusProperty::Ok, "Transform",
            "Transform OK");

  // Offset is expressed in the reference frame's own local axes, so rotate
  // it into world space before adding it to the frame's world position.
  const Ogre::Vector3 offset = position_offset_property_->getVector();
  const Ogre::Vector3 rpy_deg = rpy_offset_property_->getVector();

  const Ogre::Quaternion offset_orientation =
      Ogre::Quaternion(Ogre::Degree(rpy_deg.z), Ogre::Vector3::UNIT_Z) *
      Ogre::Quaternion(Ogre::Degree(rpy_deg.y), Ogre::Vector3::UNIT_Y) *
      Ogre::Quaternion(Ogre::Degree(rpy_deg.x), Ogre::Vector3::UNIT_X);

  const Ogre::Vector3 world_position =
      frame_position + frame_orientation * offset;
  const Ogre::Quaternion world_orientation =
      frame_orientation * offset_orientation;

  scene_node_->setScale(scale_property_->getVector());
  scene_node_->setPosition(world_position);
  scene_node_->setOrientation(world_orientation);
}

}  // namespace rviz_mesh_display

#include <pluginlib/class_list_macros.hpp>  // NOLINT
PLUGINLIB_EXPORT_CLASS(rviz_mesh_display::MeshDisplay, rviz_common::Display)
