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

#include "open3d/visualization/gui/Keyframer.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>

void ToggleButton(const char* str_id, bool* v) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float height = ImGui::GetFrameHeight();
    float width = height * 1.55f;
    float radius = height * 0.50f;

    ImGui::InvisibleButton(str_id, ImVec2(width, height));
    if (ImGui::IsItemClicked())
        *v = !*v;

    float t = *v ? 1.0f : 0.0f;

    ImGuiContext& g = *GImGui;
    float ANIM_SPEED = 0.08f;
    if (g.LastActiveId == g.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
    {
        float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
        t = *v ? (t_anim) : (1.0f - t_anim);
    }

    ImU32 col_bg;
    if (ImGui::IsItemHovered())
        col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.78f, 0.78f, 0.78f, 1.0f), ImVec4(0.64f, 0.83f, 0.34f, 1.0f), t));
    else
        col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), ImVec4(0.56f, 0.83f, 0.26f, 1.0f), t));

    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
    draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
}

namespace open3d {
namespace visualization {
namespace gui {

namespace {
static int g_next_keyframer_id = 1;
}

struct Keyframer::Impl {
    std::string id_;
    std::vector<std::string> targets_;
    int current_target_;
    int start_frame_;
    int end_frame_;
    bool play_status_;
    std::vector<int> keyframes_;
    int current_frame_;
    ImVec2 win_pos_;
    ImVec2 win_size_;
    std::string win_title_;
    std::function<void(int)> on_frame_changed_;
    std::function<void(int)> on_target_changed_;
    std::function<void(bool)> on_play_status_changed_;
    std::function<void(int)> on_keyframe_added_;
    std::function<void(int)> on_keyframe_removed_;
    std::function<void(int, int)> on_keyframe_moved_;
};

Keyframer::Keyframer(int x, int y, int w, int h, const std::string& title) : impl_(new Keyframer::Impl()) {
    impl_->id_ = "##keyframer_" + std::to_string(g_next_keyframer_id++);
    impl_->win_pos_ = ImVec2(x, y);
    impl_->win_size_ = ImVec2(w, h);
    impl_->win_title_ = title;
    impl_->play_status_ = false;
    impl_->start_frame_ = impl_->end_frame_ = impl_->current_frame_ = 0;
}

Keyframer::~Keyframer() {}

void Keyframer::SetOnFrameChanged(std::function<void(int)> on_frame_changed) {
    impl_->on_frame_changed_ = on_frame_changed;
}

void Keyframer::SetOnTargetChanged(std::function<void(int)> on_target_changed) {
    impl_->on_target_changed_ = on_target_changed;
}

void Keyframer::SetOnPlayStatusChanged(std::function<void(bool)> on_play_status_changed) {
    impl_->on_play_status_changed_ = on_play_status_changed;
}

void Keyframer::SetOnKeyframeAdded(std::function<void(int)> on_keyframe_added) {
    impl_->on_keyframe_added_ = on_keyframe_added;
}

void Keyframer::SetOnKeyframeRemoved(std::function<void(int)> on_keyframe_removed) {
    impl_->on_keyframe_removed_ = on_keyframe_removed;
}

void Keyframer::SetOnKeyframeMoved(std::function<void(int, int)> on_keyframe_moved) {
    impl_->on_keyframe_moved_ = on_keyframe_moved;
}

void Keyframer::SetAvailableTargets(const std::vector<std::string>& targets, int current_target) {
    impl_->targets_ = targets;
    impl_->current_target_ = current_target;
}

void Keyframer::SetKeyframes(int start_frame, int end_frame, const std::vector<int>& keyframes) {
    impl_->start_frame_ = start_frame;
    impl_->end_frame_ = end_frame;
    impl_->keyframes_ = keyframes;
}

void Keyframer::SetCurrentFrame(int current_frame) {
    impl_->current_frame_ = current_frame;
}

std::string Keyframer::GetTitle() const {
    return impl_->win_title_;
}

int Keyframer::GetCurrentFrame() const {
    return impl_->current_frame_;
}

int Keyframer::GetPreviousKeyframe(int current) const {
    if (current <= impl_->start_frame_) {
        return impl_->start_frame_;
    }
    for (int i = current - 1; i >= impl_->start_frame_; i--) {
        if (std::find(impl_->keyframes_.begin(), impl_->keyframes_.end(), i) !=
            impl_->keyframes_.end()) {
            return i;
        }
    }
    return impl_->start_frame_;
}

int Keyframer::GetNextKeyframe(int current) const {
    if (current >= impl_->end_frame_) {
        return impl_->end_frame_;
    }
    for (int i = current + 1; i <= impl_->end_frame_; i++) {
        if (std::find(impl_->keyframes_.begin(), impl_->keyframes_.end(), i) !=
            impl_->keyframes_.end()) {
            return i;
        }
    }
    return impl_->end_frame_;
}

int Keyframer::GetPreviousFrame(int current) const {
    if (current <= impl_->start_frame_ + 1) {
        return impl_->start_frame_;
    }
    return --current;
}

int Keyframer::GetNextFrame(int current) const {
    if (current >= impl_->end_frame_ - 1) {
        return impl_->end_frame_;
    }
    return ++current;
}

Size Keyframer::CalcPreferredSize(const LayoutContext& context,
                                  const Constraints& constraints) const {
    // This will not fit into the o3d managed layout system :P
    return Size(0, 0);
}

Keyframer::DrawResult Keyframer::Draw(const DrawContext& context) {
    ImGui::SetNextWindowPos(impl_->win_pos_, ImGuiCond_Once);
    ImGui::SetNextWindowSize(impl_->win_size_, ImGuiCond_Once);

    int i_new_frame = impl_->current_frame_;
    int i_new_target = impl_->current_target_;
    int i_new_moved_frame = impl_->current_frame_;
    bool b_new_play_status = impl_->play_status_;

    // Each node must have unique ID, format is '<title>##<ID>'
    ImGui::Begin((impl_->win_title_ + impl_->id_).c_str(), nullptr, ImGuiWindowFlags_NoCollapse);
    {
        // Set window size.
        auto pos = ImGui::GetWindowPos();
        SetFrame(Rect(pos.x, pos.y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight()));

        //// Line 1
        // Draw the slider.
        ImGui::PushItemWidth(float(GetFrame().width));
        ImGui::SliderInt("Frame", &i_new_frame, impl_->start_frame_, impl_->end_frame_);
        ImGui::PopItemWidth();

        // Draw the keyframes (Note the positions are w.r.t. the full window!)
        bool is_on_keyframe = false;
        const ImVec2 cur_pos = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const int padding = 5 + 2;
        float step_w = float(GetFrame().width - (2 * padding)) / float(impl_->end_frame_ - impl_->start_frame_);
        if (impl_->start_frame_ == impl_->end_frame_) {
            step_w = float(GetFrame().width - (2 * padding));
        }
        for (int i = 0; i < (int) impl_->keyframes_.size(); ++i) {
            const int frame = impl_->keyframes_[i];
            const float x = float(padding + (frame - impl_->start_frame_) * step_w);
            ImColor color = ImColor(0.5f, 0.0f, 0.0f);
            if (i_new_frame == frame) {
                color = ImColor(1.0f, 1.0f, 0.0f);
                is_on_keyframe = true;
            }
            draw_list->AddCircleFilled(ImVec2(cur_pos.x + x, cur_pos.y - 15), 3.0f, color, 32);
        }

        //// Line 2
        // Draw controller line.
        float combo_width = float(GetFrame().width) * 0.3;
        float avail = ImGui::GetContentRegionAvail().x;
        float off = (avail - (combo_width + 185.0f)) * 0.5f;        // Center line.
        if (off > 0.0f)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

        ImGui::PushItemWidth(combo_width);
        if (ImGui::BeginCombo("##Target", impl_->targets_[impl_->current_target_].c_str())) {
            for (int i = 0; i < (int) impl_->targets_.size(); ++i) {
                bool is_selected = (i == impl_->current_target_);
                if (ImGui::Selectable(impl_->targets_[i].c_str(), is_selected)) {
                    i_new_target = i;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        // Draw button set
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        ImGui::SameLine();
        if (ImGui::Button("<<")) {
            i_new_frame = this->GetPreviousKeyframe(impl_->current_frame_);
        }
        ImGui::SameLine();
        if (ImGui::Button("<")) {
            i_new_frame = this->GetPreviousFrame(impl_->current_frame_);
        }
        ImGui::SameLine();
        ToggleButton("Play", &b_new_play_status);
        ImGui::SameLine();
        if (ImGui::Button(">")) {
            i_new_frame = this->GetNextFrame(impl_->current_frame_);
        }
        ImGui::SameLine();
        if (ImGui::Button(">>")) {
            i_new_frame = this->GetNextKeyframe(impl_->current_frame_);
        }
        ImGui::SameLine();
        if (is_on_keyframe) {
            if (ImGui::Button("-")) {
                if (impl_->on_keyframe_removed_) {
                    impl_->on_keyframe_removed_(i_new_frame);
                    ImGui::End();
                    return Widget::DrawResult::REDRAW;
                }
            }
        } else {
            if (ImGui::Button("+")) {
                if (impl_->on_keyframe_added_) {
                    impl_->on_keyframe_added_(i_new_frame);
                    ImGui::End();
                    return Widget::DrawResult::REDRAW;
                }
            }
        }

        //// Line 3
        if (!is_on_keyframe) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        ImGui::PushItemWidth(float(GetFrame().width));
        ImGui::SliderInt("##FrameMover", &i_new_moved_frame, impl_->start_frame_, impl_->end_frame_);
        ImGui::PopItemWidth();
        if (!is_on_keyframe) {
            ImGui::PopStyleVar();
            ImGui::PopItemFlag();
        }
    }
    ImGui::End();   // End of the new window.

    if (impl_->current_frame_ != i_new_frame) {
        impl_->current_frame_ = i_new_frame;
        if (impl_->on_frame_changed_) {
            impl_->on_frame_changed_(i_new_frame);
        }
        return Widget::DrawResult::REDRAW;
    }
    if (impl_->current_target_ != i_new_target) {
        impl_->current_target_ = i_new_target;
        if (impl_->on_target_changed_) {
            impl_->on_target_changed_(i_new_target);
        }
        return Widget::DrawResult::REDRAW;
    }
    if (impl_->current_frame_ != i_new_moved_frame) {
        if (impl_->on_keyframe_moved_) {
            impl_->on_keyframe_moved_(impl_->current_frame_, i_new_moved_frame);
        }
        return Widget::DrawResult::REDRAW;
    }
    if (impl_->play_status_ != b_new_play_status) {
        impl_->play_status_ = b_new_play_status;
        if (impl_->on_play_status_changed_) {
            impl_->on_play_status_changed_(b_new_play_status);
        }
        return Widget::DrawResult::REDRAW;
    }

    return Widget::DrawResult::NONE;
}

}  // namespace gui
}  // namespace visualization
}  // namespace open3d
