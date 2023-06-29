// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// Copyright (c) 2018-2023 www.open3d.org
// SPDX-License-Identifier: MIT
// ----------------------------------------------------------------------------

#pragma once

#include "pybind/open3d_pybind.h"
#include "open3d/visualization/gui/Widget.h"
#include "open3d/visualization/gui/SceneWidget.h"

namespace open3d {
namespace geometry {
class Image;
}

namespace visualization {
namespace rendering {
class Open3DScene;
}

namespace gui {


enum class EventCallbackResult { IGNORED = 0, HANDLED, CONSUMED };

    class PySceneWidget : public SceneWidget {
        using Super = SceneWidget;

    public:
        void SetOnMouse(std::function<int(const MouseEvent &)> f) {
            on_mouse_ = f;
        }
        void SetOnKey(std::function<int(const KeyEvent &)> f) { on_key_ = f; }

        Widget::EventResult Mouse(const MouseEvent &e) override {
            if (on_mouse_) {
                switch (EventCallbackResult(on_mouse_(e))) {
                    case EventCallbackResult::CONSUMED:
                        return Widget::EventResult::CONSUMED;
                    case EventCallbackResult::HANDLED: {
                        auto result = Super::Mouse(e);
                        if (result == Widget::EventResult::IGNORED) {
                            result = Widget::EventResult::CONSUMED;
                        }
                        return result;
                    }
                    case EventCallbackResult::IGNORED:
                    default:
                        return Super::Mouse(e);
                }
            } else {
                return Super::Mouse(e);
            }
        }

        Widget::EventResult Key(const KeyEvent &e) override {
            if (on_key_) {
                switch (EventCallbackResult(on_key_(e))) {
                    case EventCallbackResult::CONSUMED:
                        return Widget::EventResult::CONSUMED;
                    case EventCallbackResult::HANDLED: {
                        auto result = Super::Key(e);
                        if (result == Widget::EventResult::IGNORED) {
                            result = Widget::EventResult::CONSUMED;
                        }
                        return result;
                    }
                    case EventCallbackResult::IGNORED:
                    default:
                        return Super::Key(e);
                }
            } else {
                return Super::Key(e);
            }
        }

    private:
        std::function<int(const MouseEvent &)> on_mouse_;
        std::function<int(const KeyEvent &)> on_key_;
    };

void InitializeForPython(std::string resource_path = "", bool headless = false);
std::shared_ptr<geometry::Image> RenderToImageWithoutWindow(
        rendering::Open3DScene *scene, int width, int height);
std::shared_ptr<geometry::Image> RenderToDepthImageWithoutWindow(
        rendering::Open3DScene *scene,
        int width,
        int height,
        bool z_in_view_space = false);

void pybind_gui(py::module &m);

void pybind_gui_events(py::module &m);
void pybind_gui_classes(py::module &m);

}  // namespace gui
}  // namespace visualization
}  // namespace open3d
