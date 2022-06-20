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

#include <functional>
#include <Eigen/Dense>

#include "open3d/visualization/gui/Widget.h"

namespace open3d {
namespace visualization {
namespace gui {

class Keyframer : public Widget {
public:
    Keyframer(int x, int y, int w, int h, const std::string& title);
    ~Keyframer() override;

    void SetAvailableTargets(const std::vector<std::string>& targets, int current_target);
    // Note: end_frame is inclusive.
    void SetKeyframes(int start_frame, int end_frame, const std::vector<int>& keyframes);
    void SetCurrentFrame(int current_frame);

    std::string GetTitle() const;
    int GetCurrentFrame() const;

    int GetPreviousKeyframe(int current) const;
    int GetNextKeyframe(int current) const;

    int GetPreviousFrame(int current) const;
    int GetNextFrame(int current) const;

    Size CalcPreferredSize(const LayoutContext& context,
                           const Constraints& constraints) const override;

    DrawResult Draw(const DrawContext& context) override;

    /// Specifies a callback function which will be called when the value
    /// changes as a result of user action.
    void SetOnFrameChanged(std::function<void(int)> on_frame_changed);
    void SetOnTargetChanged(std::function<void(int)> on_target_changed);
    void SetOnPlayStatusChanged(std::function<void(bool)> on_play_status_changed);
    void SetOnKeyframeAdded(std::function<void(int)> on_keyframe_added);
    void SetOnKeyframeRemoved(std::function<void(int)> on_keyframe_removed);
    void SetOnKeyframeMoved(std::function<void(int, int)> on_keyframe_moved);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace gui
}  // namespace visualization
}  // namespace open3d
