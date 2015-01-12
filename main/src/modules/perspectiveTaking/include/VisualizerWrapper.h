/*
 *
 * Copyright (C) 2015 WYSIWYD Consortium, European Commission FP7 Project ICT-612139
 * Authors: Tobias Fischer
 * email:   t.fischer@imperial.ac.uk
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * wysiwyd/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

#ifndef VPT_VISUALIZER_WRAPPER
#define VPT_VISUALIZER_WRAPPER

#include <string>
#include <memory>
#include <map>

#include <pcl/pcl_base.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/PolygonMesh.h>
#include <pcl/PCLPointCloud2.h>
#include <pcl/visualization/pcl_visualizer.h>

#include <rtabmap/core/Transform.h>

using namespace rtabmap;

class VColor {
public:
    VColor();
    VColor(unsigned char red, unsigned char green, unsigned char blue) :
        r(red),
        g(green),
        b(blue)
    {
    }
    virtual ~VColor() {}
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

class VisualizerWrapper {
public:
    VisualizerWrapper();
    virtual ~VisualizerWrapper();

    const std::map<std::string, Transform> & getAddedClouds() const {return _addedClouds;}

    void setBackgroundColor(const VColor &color);

    bool getPose(const std::string & id, Transform & pose); //including meshes

    bool updateCloudPose(
            const std::string & id,
            const Transform & pose); //including mesh

    void setCloudVisibility(const std::string & id, bool isVisible);

    void removeAllClouds(); //including meshes

    bool removeCloud(const std::string & id); //including mesh

    bool updateCloud(
            const std::string & id,
            const pcl::PointCloud<pcl::PointXYZRGB>::Ptr & cloud,
            const Transform & pose = Transform::getIdentity());

    bool updateCloud(
            const std::string & id,
            const pcl::PointCloud<pcl::PointXYZ>::Ptr & cloud,
            const Transform & pose = Transform::getIdentity());

    bool addOrUpdateCloud(
            const std::string & id,
            const pcl::PointCloud<pcl::PointXYZRGB>::Ptr & cloud,
            const Transform & pose = Transform::getIdentity(),
            const VColor & color = _vgrey);

    bool addOrUpdateCloud(
            const std::string & id,
            const pcl::PointCloud<pcl::PointXYZ>::Ptr & cloud,
            const Transform & pose = Transform::getIdentity(),
            const VColor & color = _vgrey);

    bool addCloud(
            const std::string & id,
            const pcl::PCLPointCloud2Ptr & binaryCloud,
            const Transform & pose,
            bool rgb,
            const VColor & color = _vgrey);

    bool addCloud(
            const std::string & id,
            const pcl::PointCloud<pcl::PointXYZRGB>::Ptr & cloud,
            const Transform & pose = Transform::getIdentity(),
            const VColor & color = _vgrey);

    bool addCloud(
            const std::string & id,
            const pcl::PointCloud<pcl::PointXYZ>::Ptr & cloud,
            const Transform & pose = Transform::getIdentity(),
            const VColor & color = _vgrey);

    void updateCameraPosition(
            const Transform & pose);

    pcl::visualization::PCLVisualizer& getVisualizer() {
        return *_visualizer;
    }

    std::map<std::string, int> getViewports() {
        return _viewports;
    }

private:
    std::map<std::string, Transform> _addedClouds;
    unsigned int _maxTrajectorySize;
    Transform _lastPose;
    pcl::PointCloud<pcl::PointXYZ>::Ptr _trajectory;
    static const VColor _vgrey;
    pcl::visualization::PCLVisualizer* _visualizer;
    std::map<std::string, int> _viewports;
};

#endif