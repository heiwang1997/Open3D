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

void InitializeForPython(std::string resource_path = "");
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
