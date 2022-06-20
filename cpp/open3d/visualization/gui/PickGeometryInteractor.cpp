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

#include "open3d/visualization/gui/PickGeometryInteractor.h"

#include <unordered_map>
#include <unordered_set>

#include "open3d/geometry/Image.h"
#include "open3d/geometry/PointCloud.h"
#include "open3d/geometry/TriangleMesh.h"
#include "open3d/t/geometry/PointCloud.h"
#include "open3d/t/geometry/TriangleMesh.h"
#include "open3d/utility/Logging.h"
#include "open3d/visualization/gui/Events.h"
#include "open3d/visualization/rendering/MaterialRecord.h"
#include "open3d/visualization/rendering/Open3DScene.h"
#include "open3d/visualization/rendering/Scene.h"
#include "open3d/visualization/rendering/View.h"

#define WANT_DEBUG_IMAGE 0

#if WANT_DEBUG_IMAGE
#include "open3d/io/ImageIO.h"
#endif  // WANT_DEBUG_IMAGE

namespace open3d {
namespace visualization {
namespace gui {

namespace {
// Background color is white, so that index 0 can be black
static const Eigen::Vector4f kBackgroundColor = {1.0f, 1.0f, 1.0f, 1.0f};
static const unsigned int kMaxPickableIndex = 0x00fffffd;

inline bool IsValidIndex(uint32_t idx) { return (idx <= kMaxPickableIndex); }

Eigen::Vector3d CalcIndexColor(uint32_t idx) {
    const double red = double((idx & 0x00ff0000) >> 16) / 255.0;
    const double green = double((idx & 0x0000ff00) >> 8) / 255.0;
    const double blue = double((idx & 0x000000ff)) / 255.0;
    return {red, green, blue};
}

Eigen::Vector3d SetColorForIndex(uint32_t idx) {
    return CalcIndexColor(std::min(kMaxPickableIndex, idx));
}

uint32_t GetIndexForColor(geometry::Image *image, int x, int y) {
    uint8_t *rgb = image->PointerAt<uint8_t>(x, y, 0);
    const unsigned int red = (static_cast<unsigned int>(rgb[0]) << 16);
    const unsigned int green = (static_cast<unsigned int>(rgb[1]) << 8);
    const unsigned int blue = (static_cast<unsigned int>(rgb[2]));
    return (red | green | blue);
}

}  // namespace

// ----------------------------------------------------------------------------
class GeometryLUT {
private:
    struct Obj {
        std::string name;
        size_t index;

        Obj(const std::string &n, size_t idx) : name(n), index(idx) {};
        bool IsValid() const { return !name.empty(); }
    };

public:
    void Clear() { objects_.clear(); }

    // index must be larger than all previously added items
    void Add(const std::string &name, size_t idx) {
        if (!objects_.empty() && objects_.back().index >= idx) {
            utility::LogError(
                    "start_index {} must be larger than all previously added "
                    "objects {}.",
                    idx, objects_.back().index);
        }
        objects_.emplace_back(name, idx);
    }

    const Obj &ObjectForIndex(size_t index) {
        if (objects_.size() == 1) {
            return objects_[0];
        } else {
            auto next = std::upper_bound(objects_.begin(), objects_.end(),
                                         index, [](size_t value, const Obj &o) {
                                             return value < o.index;
                                         });
            if (next == objects_.end()) {
                return objects_.back();
            } else {
                --next;
                return *next;
            }
        }
    }

    int size() const {
        return (int) objects_.size();
    }

private:
    std::vector<Obj> objects_;
};

// ----------------------------------------------------------------------------
PickGeometryInteractor::PickGeometryInteractor(rendering::Open3DScene *scene,
                                               rendering::Camera *camera) {
    scene_ = scene;
    camera_ = camera;

    // This scene is used to render the picking image (color indicates object index).
    picking_scene_ = std::make_shared<rendering::Open3DScene>(scene->GetRenderer());

    picking_scene_->SetDownsampleThreshold(SIZE_MAX);
    picking_scene_->SetBackground(kBackgroundColor);

    picking_scene_->GetView()->ConfigureForColorPicking();
}

PickGeometryInteractor::~PickGeometryInteractor() { delete lookup_; }

void PickGeometryInteractor::SetPickableGeometry(
        const std::vector<SceneWidget::PickableGeometry> &geometry) {
    delete lookup_;
    lookup_ = new GeometryLUT();

    picking_scene_->ClearGeometry();
    SetNeedsRedraw();

    int n_geometries = 0;
    for (auto &pg : geometry) {
        geometry::Geometry3D* geom = nullptr;
        auto cloud = dynamic_cast<const geometry::PointCloud *>(pg.geometry);
        auto mesh = dynamic_cast<const geometry::TriangleMesh *>(pg.geometry);
        if (cloud) {
            auto* new_cloud = new geometry::PointCloud(cloud->points_);
            new_cloud->PaintUniformColor(SetColorForIndex(uint32_t(n_geometries)));
            new_cloud->Transform(pg.transform);
            geom = new_cloud;
        } else if (mesh) {
            auto* new_mesh = new geometry::TriangleMesh(mesh->vertices_,
                                                        mesh->triangles_);
            new_mesh->PaintUniformColor(SetColorForIndex(uint32_t(n_geometries)));
            new_mesh->Transform(pg.transform);
            geom = new_mesh;
        } else {
            continue;
        }
        auto mat = MakeMaterial();
        picking_scene_->AddGeometry(pg.name, geom, mat);
        picking_scene_->GetScene()->GeometryShadows(pg.name, false, false);
        lookup_->Add(pg.name, n_geometries++);
        delete geom;
    }

    // add safety but invalid obj
    lookup_->Add("", n_geometries);
}

void PickGeometryInteractor::SetNeedsRedraw() { dirty_ = true; }

rendering::MatrixInteractorLogic &PickGeometryInteractor::GetMatrixInteractor() {
    return matrix_logic_;
}

void PickGeometryInteractor::SetOnGeometryPicked(std::function<void(const std::vector<std::string>&)> f) {
    on_picked_ = f;
}

void PickGeometryInteractor::Mouse(const MouseEvent &e) {
    if (e.type == MouseEvent::BUTTON_UP) {
        if (e.modifiers & int(KeyModifier::ALT)) {
            // Not yet implemented.
        } else {
            rect_points_.push_back(gui::Point(e.x, e.y));
            DoPick();
        }
    }
}

void PickGeometryInteractor::Key(const KeyEvent &e) {
    if (e.type == KeyEvent::UP) {
        if (e.key == KEY_ESCAPE) {
            ClearPick();
        }
    }
}

void PickGeometryInteractor::DoPick() {
    if (dirty_) {
        SetNeedsRedraw();
        auto *view = picking_scene_->GetView();
        view->SetViewport(0, 0,  // in case scene widget changed size
                          matrix_logic_.GetViewWidth(),
                          matrix_logic_.GetViewHeight());
        view->GetCamera()->CopyFrom(camera_);
        picking_scene_->GetRenderer().RenderToImage(
                view, picking_scene_->GetScene(),
                [this](std::shared_ptr<geometry::Image> img) {
#if WANT_DEBUG_IMAGE
                    std::cout << "[debug] Writing pick image to "
                              << "debug.png" << std::endl;
                    io::WriteImage("debug.png", *img);
#endif  // WANT_DEBUG_IMAGE
                    this->OnPickImageDone(img);
                });
    } else {
        OnPickImageDone(pick_image_);
    }
}

void PickGeometryInteractor::ClearPick() {
    rect_points_.clear();
    SetNeedsRedraw();
}

rendering::MaterialRecord PickGeometryInteractor::MakeMaterial() {
    rendering::MaterialRecord mat;
    mat.shader = "unlitPolygonOffset";
    mat.point_size = float(3.);
    // We are not tonemapping, so src colors are RGB. This prevents the colors
    // being converted from sRGB -> linear like normal.
    mat.sRGB_color = false;
    return mat;
}

void PickGeometryInteractor::OnPickImageDone(
        std::shared_ptr<geometry::Image> img) {
    if (dirty_) {
        pick_image_ = img;
        dirty_ = false;
    }

    auto *imge = pick_image_.get();
    std::vector<std::string> picked;
    if (rect_points_.size() == 1) {
        const int x0 = rect_points_[0].x;
        const int y0 = rect_points_[0].y;
        struct Score {  // this is a struct to force a default value
            float score = 0;
        };
        std::unordered_map<unsigned int, Score> candidates;
        // auto clicked_idx = GetIndexForColor(imge, x0, y0);
        int radius = 5;
        for (int y = y0 - radius; y < y0 + radius; ++y) {
            for (int x = x0 - radius; x < x0 + radius; ++x) {
                unsigned int idx = GetIndexForColor(imge, x, y);
                if (IsValidIndex(idx) && idx < (unsigned int) lookup_->size()) {
                    float dist = std::sqrt(float((x - x0) * (x - x0) +
                                                    (y - y0) * (y - y0)));
                    candidates[idx].score += radius - dist;
                }
            }
        }
        if (!candidates.empty()) {
            // Note that scores are (radius - dist), and since we take from
            // a square pattern, a score can be negative. And multiple
            // pixels of a point scoring negatively can make the negative up
            // to -point_size^2.
            float best_score = -1e30f;
            unsigned int best_idx = (unsigned int)-1;
            for (auto &idx_score : candidates) {
                if (idx_score.second.score > best_score) {
                    best_score = idx_score.second.score;
                    best_idx = idx_score.first;
                }
            }
            auto &o = lookup_->ObjectForIndex(best_idx);
            if (o.IsValid()) {
                picked.push_back(o.name);
            }
        }
    } else {
        // Rect selection. Not yet implemented.
    }

    // Call the callback even if we didn't pick anything.
    if (on_picked_) {
        on_picked_(picked);
    }
    ClearPick();
}

}  // namespace gui
}  // namespace visualization
}  // namespace open3d
