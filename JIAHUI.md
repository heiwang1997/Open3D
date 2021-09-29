# Jiahui's fork of Open3D

## NOTE

I will work on my master branch, and merge with upstream using the following command.
List all files without untracked ones with `gst -uno`

## Merging with upstream head

1. Commit your changes.
2. `git pull upstream master`
3. Commit the merge.
4. (Optionally) `git push origin master`

## Compile

Please follow: http://www.open3d.org/docs/release/compilation.html

Or simply do:
```bash
cd build
cmake ..
make -j
ca
make install-pip-package
```

## Modifications compared

`git diff <HEAD@upstream>`

1. Add camera binding.
2. Add set_on_key for `gui.Window`, so that `gui.O3DVisualizer`, as a special case of window, can work.
    - For the callbacks please set the following values: 0-IGNORED (other operations should not continue), 1-HANDLED (other operations should continue)
3. Add `o3dvisualizer.scene_widget` python binding to get the SceneWidget within the pre-built visualizer.
    - The original scene widget in the visualizer is a SceneWidget, which cannot be converted into python type. So I created a new parameter in the constructor of O3DVisualizer to accept an external widget pointer, which we pass in a PySceneWidget pointer.
    - In order to achieve this, we move PySceneWidget definition from `pybind/visualization/gui/gui.cpp` to `gui.h`, so that it can be referenced in `pybind/visualization/o3dvisualizer.cpp` and create the python compatible constructor.
4. Add `register_view_refresh_callback` into the legacy `o3d.Visualizer` class, in order to capture camera moving events (to sync views from different windows).
5. Add `set_shadowing(bool, ShadowType)` binding to `FilamentView` class, so that in python we can change the method to compute shadows.
6. Fix a bug in `rendering.filament.FilamentView::CopySettingsFrom`, so that color grading can also be copied. Otherwise during rendering to image, the rendered image will not have the same cg as the GUI one.
