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

#include "open3d/visualization/gui/Histogram.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
#include <implot_internal.h>

#include <cmath>

#include "open3d/visualization/gui/Theme.h"

// Expose more implot functions that is helpful to draw ramp histogram.
namespace ImPlot {
bool BeginItem(const char* label_id, ImPlotItemFlags flags, ImPlotCol recolor_from);
void EndItem();
ImU32 SampleColormapU32(float t, ImPlotColormap cmap);
}

namespace open3d {
namespace visualization {
namespace gui {

namespace {
static int g_next_histogram_id = 1;
}

struct Histogram::Impl {
    std::string id_;
    float v_max_;
    float v_min_;
    Eigen::ArrayXf values_;
    ImPlotColormap_ colormap_;
    ImVec2 win_pos_;
    ImVec2 win_size_;
    std::string win_title_;
};

Histogram::Histogram(int x, int y, int w, int h, const std::string& title) : impl_(new Histogram::Impl()) {
    impl_->id_ = "##histogram_" + std::to_string(g_next_histogram_id++);
    impl_->win_pos_ = ImVec2(x, y);
    impl_->win_size_ = ImVec2(w, h);
    impl_->win_title_ = title;
}

Histogram::~Histogram() {}

void Histogram::SetValue(float v_min, float v_max, const Eigen::ArrayXf& value, Color::Colormap colormap) {
    impl_->v_min_ = v_min;
    impl_->v_max_ = v_max;
    impl_->values_ = value;
    switch (colormap) {
        case Color::Colormap::VIRIDIS: impl_->colormap_ = ImPlotColormap_Viridis; break;
        case Color::Colormap::PLASMA: impl_->colormap_ = ImPlotColormap_Plasma; break;
        case Color::Colormap::JET: impl_->colormap_ = ImPlotColormap_Jet; break;
        case Color::Colormap::SPECTRAL: impl_->colormap_ = ImPlotColormap_Spectral; break;
        default: impl_->colormap_ = ImPlotColormap_Deep; break;
    }
}

const Eigen::ArrayXf& Histogram::GetValue() const { 
    return impl_->values_; 
}

std::string Histogram::GetTitle() const {
    return impl_->win_title_;
}

Size Histogram::CalcPreferredSize(const LayoutContext& context,
                                  const Constraints& constraints) const {
    // This will not fit into the o3d managed layout system :P
    return Size(0, 0);
}

Histogram::DrawResult Histogram::Draw(const DrawContext& context) {

    // This widget is naughty and won't fit in the context
    //  It create its own sub-window!
    // This is because O3DVisualizer doesn't support create a floating child yet.

    ImGui::SetNextWindowPos(impl_->win_pos_, ImGuiCond_Once);
    ImGui::SetNextWindowSize(impl_->win_size_, ImGuiCond_Once);
    // Each node must have unique ID, format is '<title>##<ID>'
    ImGui::Begin((impl_->win_title_ + impl_->id_).c_str(), nullptr, ImGuiWindowFlags_NoCollapse);
    
    if (ImPlot::BeginPlot("Distribution##Histograms", 
        ImGui::GetContentRegionAvail(),
        ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoBoxSelect
    )) {
        ImPlot::SetupAxis(ImAxis_X1, 0, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxis(ImAxis_Y1, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels);
        if (ImPlot::BeginItem("PC1", 0, ImPlotCol_Fill)) {
            ImDrawList& DrawList = *ImPlot::GetPlotDrawList();
            int n_bars = impl_->values_.rows();
            const double half_width = (impl_->v_max_ - impl_->v_min_) / (n_bars - 1) / 2;

            // Used for auto-fit.
            if (ImPlot::FitThisFrame()) {
                for (int i = 0; i < n_bars; ++i) {
                    float alpha = (float)i / (n_bars - 1);
                    ImPlotPoint p(impl_->v_min_ * (1 - alpha) + impl_->v_max_ * alpha, impl_->values_(i));
                    ImPlot::FitPoint(ImPlotPoint(p.x - half_width, p.y));
                    ImPlot::FitPoint(ImPlotPoint(p.x + half_width, 0));
                }
            }

            for (int i = 0; i < n_bars; ++i) {
                float alpha = (float)i / (n_bars - 1);
                ImU32 col_fill = ImPlot::SampleColormapU32(alpha, impl_->colormap_);
                ImPlotPoint p(impl_->v_min_ * (1 - alpha) + impl_->v_max_ * alpha, impl_->values_(i));
                if (p.y == 0) continue;
                ImVec2 a = ImPlot::PlotToPixels(p.x - half_width, p.y);
                ImVec2 b = ImPlot::PlotToPixels(p.x + half_width, 0);
                float width_px = ImAbs(a.x-b.x);
                if (width_px < 1.0f) {
                    a.x += a.x > b.x ? (1-width_px) / 2 : (width_px-1) / 2;
                    b.x += b.x > a.x ? (1-width_px) / 2 : (width_px-1) / 2;
                }
                DrawList.AddRectFilled(a, b, col_fill);
            }
            ImPlot::EndItem();
        }
        ImPlot::EndPlot();
    }

    // Set window size.
    auto pos = ImGui::GetWindowPos();
    SetFrame(Rect(pos.x, pos.y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight()));

    ImGui::End();   // End of the new window.

    return Widget::DrawResult::NONE;
}

}  // namespace gui
}  // namespace visualization
}  // namespace open3d
