/*
 * Copyright (c) 2015, Georgia Tech Research Corporation
 * All rights reserved.
 *
 * Author(s): Michael X. Grey <mxgrey@gatech.edu>
 *
 * Georgia Tech Graphics Lab and Humanoid Robotics Lab
 *
 * Directed by Prof. C. Karen Liu and Prof. Mike Stilman
 * <karenliu@cc.gatech.edu> <mstilman@cc.gatech.edu>
 *
 * This file is provided under the following "BSD-style" License:
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 */

#include "dart/dynamics/Skeleton.h"
#include "dart/dynamics/BodyNode.h"
#include "dart/dynamics/SimpleFrame.h"
#include "dart/dynamics/EllipsoidShape.h"
#include "dart/math/Helpers.h"

#include "SupportPolygonVisual.h"

namespace osgDart {

//==============================================================================
SupportPolygonVisual::SupportPolygonVisual(const dart::dynamics::SkeletonPtr& skeleton,
                             double elevation)
  : mSkeleton(skeleton),
    mTreeIndex(dart::dynamics::INVALID_INDEX),
    mElevation(elevation)
{
  initialize();
}

//==============================================================================
SupportPolygonVisual::SupportPolygonVisual(const dart::dynamics::SkeletonPtr& skeleton,
                             size_t treeIndex, double elevation)
  : mSkeleton(skeleton),
    mTreeIndex(treeIndex),
    mElevation(elevation)
{
  initialize();
}

//==============================================================================
void SupportPolygonVisual::setSkeleton(const dart::dynamics::SkeletonPtr& skeleton)
{
  mSkeleton = skeleton;
}

//==============================================================================
dart::dynamics::SkeletonPtr SupportPolygonVisual::getSkeleton() const
{
  return mSkeleton.lock();
}

//==============================================================================
void SupportPolygonVisual::visualizeWholeSkeleton()
{
  mTreeIndex = dart::dynamics::INVALID_INDEX;
}

//==============================================================================
void SupportPolygonVisual::visualizeTree(size_t treeIndex)
{
  mTreeIndex = treeIndex;
}

//==============================================================================
void SupportPolygonVisual::setDisplayElevation(double elevation)
{
  mElevation = elevation;
}

//==============================================================================
double SupportPolygonVisual::getDisplayElevation() const
{
  return mElevation;
}

//==============================================================================
void SupportPolygonVisual::setPolygonColor(const Eigen::Vector4d& color)
{
  (*mPolygonColor)[0] = osg::Vec4(color[0], color[1], color[2], color[3]);
}

//==============================================================================
Eigen::Vector4d SupportPolygonVisual::getPolygonColor() const
{
  const osg::Vec4& c = (*mPolygonColor)[0];
  return Eigen::Vector4d(c[0], c[1], c[2], c[3]);
}

//==============================================================================
void SupportPolygonVisual::displayCentroid(bool display)
{
  if(mDisplayCentroid == display)
    return;

  mDisplayCentroid = display;

  if(mDisplayCentroid)
    addChild(mCentroidNode);
  else
    removeChild(mCentroidNode);
}

//==============================================================================
bool SupportPolygonVisual::isCentroidDisplayed() const
{
  return mDisplayCentroid;
}

//==============================================================================
void SupportPolygonVisual::setCentroidRadius(double radius)
{
  if(mCentroidRadius == radius)
    return;

  mCentroidRadius = radius;
  const dart::dynamics::ShapePtr& shape = mCentroid->getVisualizationShape(0);
  std::static_pointer_cast<dart::dynamics::EllipsoidShape>(shape)->setSize(
        mCentroidRadius/2.0*Eigen::Vector3d::Ones());
  shape->addDataVariance(dart::dynamics::Shape::DYNAMIC_PRIMITIVE);
}

//==============================================================================
double SupportPolygonVisual::getCentroidRadius() const
{
  return mCentroidRadius;
}

//==============================================================================
void SupportPolygonVisual::displayCenterOfMass(bool display)
{
  if(mDisplayCOM == display)
    return;

  mDisplayCOM = display;

  if(mDisplayCOM)
    addChild(mComNode);
  else
    removeChild(mComNode);
}

//==============================================================================
bool SupportPolygonVisual::isCenterOfMassDisplayed() const
{
  return mDisplayCOM;
}

//==============================================================================
void SupportPolygonVisual::setCenterOfMassRadius(double radius)
{
  if(mComRadius == radius)
    return;

  mComRadius = radius;
  const dart::dynamics::ShapePtr& shape = mCom->getVisualizationShape(0);
  std::static_pointer_cast<dart::dynamics::EllipsoidShape>(shape)->setSize(
        mComRadius/2.0*Eigen::Vector3d::Ones());
  shape->addDataVariance(dart::dynamics::Shape::DYNAMIC_PRIMITIVE);
}

//==============================================================================
double SupportPolygonVisual::getCenterOfMassRadius() const
{
  return mComRadius;
}

//==============================================================================
void SupportPolygonVisual::setValidCOMColor(const Eigen::Vector4d& color)
{
  mValidColor = color;
}

//==============================================================================
const Eigen::Vector4d& SupportPolygonVisual::getValidCOMColor() const
{
  return mValidColor;
}

//==============================================================================
void SupportPolygonVisual::setInvalidCOMColor(const Eigen::Vector4d& color)
{
  mInvalidColor = color;
}

//==============================================================================
const Eigen::Vector4d& SupportPolygonVisual::getInvalidCOMColor() const
{
  return mInvalidColor;
}

//==============================================================================
void SupportPolygonVisual::refresh()
{
  const dart::dynamics::SkeletonPtr& skel = mSkeleton.lock();
  if(nullptr == skel)
    return;

  const dart::math::SupportPolygon& poly =
      (dart::dynamics::INVALID_INDEX == mTreeIndex)?
        skel->getSupportPolygon() : skel->getSupportPolygon(mTreeIndex);

  const std::pair<Eigen::Vector3d, Eigen::Vector3d>& axes =
      (dart::dynamics::INVALID_INDEX == mTreeIndex)?
        skel->getSupportAxes() : skel->getSupportAxes(mTreeIndex);
  const Eigen::Vector3d& up = axes.first.cross(axes.second);

  mVertices->resize(poly.size());
  mFaces->resize(poly.size());
  for(size_t i=0; i < poly.size(); ++i)
  {
    const Eigen::Vector3d& v = axes.first*poly[i][0] + axes.second*poly[i][1]
                                + up*mElevation;
    (*mVertices)[i] = osg::Vec3(v[0], v[1], v[2]);
    (*mFaces)[i] = i;
  }

  mGeom->setVertexArray(mVertices);
  mGeom->setPrimitiveSet(0, mFaces);

  mGeom->setColorArray(mPolygonColor, osg::Array::BIND_OVERALL);

  if(mDisplayCentroid)
  {
    const Eigen::Vector2d& Cp = (dart::dynamics::INVALID_INDEX == mTreeIndex)?
          skel->getSupportCentroid() : skel->getSupportCentroid(mTreeIndex);

    const Eigen::Vector3d& C = Cp[0]*axes.first + Cp[1]*axes.second
                               + up*mElevation;

    Eigen::Isometry3d tf(Eigen::Isometry3d::Identity());
    tf.translation() = C;
    mCentroid->setTransform(tf);

    mCentroidNode->refresh(false, true);

    // Turn off primitive variance each cycle to avoid unnecessary re-updating
    mCentroid->getVisualizationShape(0)->removeDataVariance(
          dart::dynamics::Shape::DYNAMIC_PRIMITIVE);
  }

  if(mDisplayCOM)
  {
    Eigen::Vector3d com(Eigen::Vector3d::Zero());
    if(dart::dynamics::INVALID_INDEX == mTreeIndex)
    {
      com = skel->getCOM();
    }
    else
    {
      // We need to calculate tree COM ourselves, because that is not provided
      // by the API (yet)
      const std::vector<dart::dynamics::BodyNode*>& bns =
          skel->getTreeBodyNodes(mTreeIndex);

      double mass = 0.0;
      for(size_t i=0; i < bns.size(); ++i)
      {
        dart::dynamics::BodyNode* bn = bns[i];
        com += bn->getMass() * bn->getCOM();
        mass += bn->getMass();
      }

      assert(mass != 0.0);
      com = com/mass;
    }

    const Eigen::Vector2d& Cproj = Eigen::Vector2d(com.dot(axes.first),
                                                   com.dot(axes.second));
    const Eigen::Vector3d& C = Cproj[0]*axes.first
                             + Cproj[1]*axes.second
                             + up*mElevation;

    Eigen::Isometry3d tf(Eigen::Isometry3d::Identity());
    tf.translation() = C;
    mCom->setTransform(tf);

    if(dart::math::isInsideSupportPolygon(Cproj, poly))
      mCom->getVisualizationShape(0)->setColor(mValidColor);
    else
      mCom->getVisualizationShape(0)->setColor(mInvalidColor);

    mComNode->refresh(false, true);

    // Turn off primitive variance each cycle to avoid unnecessary re-updating
    mCom->getVisualizationShape(0)->removeDataVariance(
          dart::dynamics::Shape::DYNAMIC_PRIMITIVE);
  }
}

//==============================================================================
void SupportPolygonVisual::initialize()
{
  mGeode = new osg::Geode;
  mGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  addChild(mGeode);

  mGeom = new osg::Geometry;
  mGeode->addDrawable(mGeom);

  mVertices = new osg::Vec3Array;
  mGeom->setVertexArray(mVertices);
  mGeom->setDataVariance(osg::Object::DYNAMIC);

  osg::Vec4 color(0.1, 0.9, 0.1, 1.0);
  mPolygonColor = new osg::Vec4Array;
  mPolygonColor->resize(1);
  (*mPolygonColor)[0] = color;
  mGeom->setColorArray(mPolygonColor);
  mGeom->setColorBinding(osg::Geometry::BIND_OVERALL);

  mFaces = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_FAN, 0);
  mGeom->addPrimitiveSet(mFaces);

  mDisplayCentroid = true;
  mCentroid = std::make_shared<dart::dynamics::SimpleFrame>(
        dart::dynamics::Frame::World(), "centroid");

  mCentroidRadius = 0.12;
  mCentroid->addVisualizationShape(
        std::make_shared<dart::dynamics::EllipsoidShape>(
          mCentroidRadius/2.0*Eigen::Vector3d::Ones()));

  mCentroid->getVisualizationShape(0)->setColor(
        Eigen::Vector4d(color[0], color[1], color[2], color[3]));

  mCentroidNode = new FrameNode(mCentroid.get(), nullptr, false, false);
  addChild(mCentroidNode);

  mValidColor = dart::Color::Blue(1.0);
  mInvalidColor = dart::Color::Red(1.0);

  mDisplayCOM = true;
  mCom = std::make_shared<dart::dynamics::SimpleFrame>(
        dart::dynamics::Frame::World(), "com");

  mComRadius = mCentroidRadius;
  mCom->addVisualizationShape(std::make_shared<dart::dynamics::EllipsoidShape>(
                                mComRadius/2.0*Eigen::Vector3d::Ones()));
  mCom->getVisualizationShape(0)->addDataVariance(
        dart::dynamics::Shape::DYNAMIC_COLOR);

  mComNode = new FrameNode(mCom.get(), nullptr, false, false);
  addChild(mComNode);
}

} // namespace osgDart
