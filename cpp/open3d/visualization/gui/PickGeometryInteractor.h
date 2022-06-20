// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018-2021 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#pragma once

#include <queue>

#include "open3d/visualization/gui/SceneWidget.h"
#include "open3d/visualization/rendering/MatrixInteractorLogic.h"

namespace open3d {

namespace geometry {
class Geometry3D;
class Image;
}  // namespace geometry

namespace visualization {

namespace rendering {
class Camera;
struct MaterialRecord;
class MatrixInteractorLogic;
class Open3DScene;
}  // namespace rendering

namespace gui {

class GeometryLUT;

// This is an internal class used by SceneWidget
class PickGeometryInteractor : public SceneWidget::MouseInteractor {
public:
    PickGeometryInteractor(rendering::Open3DScene* scene,
                         rendering::Camera* camera);
    virtual ~PickGeometryInteractor();

    /// Sets the points that can be picked. Limited to 16 million or less
    /// points/vertices total. Geometry pointers will not be cached.
    void SetPickableGeometry(
            const std::vector<SceneWidget::PickableGeometry>& geometry);

    /// Indicates that the selection scene must be redrawn and the picking
    /// pixels retrieved again before picking.
    void SetNeedsRedraw();

    /// Calls the provided function when points are picked:
    ///    f(indices, key_modifiers)
    void SetOnGeometryPicked(std::function<void(const std::vector<std::string>&)> f);

    void DoPick();
    void ClearPick();

    rendering::MatrixInteractorLogic& GetMatrixInteractor() override;
    void Mouse(const MouseEvent& e) override;
    void Key(const KeyEvent& e) override;

protected:
    void OnPickImageDone(std::shared_ptr<geometry::Image> img);

    rendering::MaterialRecord MakeMaterial();

private:
    rendering::Open3DScene* scene_;
    rendering::Camera* camera_;

    std::function<void(const std::vector<std::string>&)> on_picked_;
    
    rendering::MatrixInteractorLogic matrix_logic_;
    std::shared_ptr<rendering::Open3DScene> picking_scene_;

    // This is a pointer rather than unique_ptr so that we don't have
    // to define this (internal) class in the header file.
    GeometryLUT* lookup_ = nullptr;
    std::shared_ptr<geometry::Image> pick_image_;
    bool dirty_ = true;

    // size = 1 if clicking, size = 2 if dragging
    std::vector<gui::Point> rect_points_;
};

}  // namespace gui
}  // namespace visualization
}  // namespace open3d
