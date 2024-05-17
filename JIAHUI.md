# Jiahui's fork of Open3D

I will work on my master branch, and merge with upstream using the following command.

## Merging with upstream head

1. Commit your changes.
2. `git pull upstream main`
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

## Public Package

To build public package, just run (currently support 38/39/310/311):
```bash
BUILD_TENSORFLOW_OPS=OFF BUILD_PYTORCH_OPS=OFF docker/docker_build.sh cuda_wheel_py310_dev  # will add commit hash
```
The final artifact would be available at the current directory.

**Clean Package Content**

I set the PyPI package name to `open3d_pycg` because this package will be very tightly used with `pycg`, and this removes collison with other versions of `open3d`. Still, there are many useless things in the packages (depencies, functions, ML, ...), hence I remove them via:

```bash
# Move all cpu packages
mv open3d_pycg_cpu*.whl whl_packages/
# Trim the package
python whl_trimmer.py whl_packages/
# Organize files
mv whl_packages/open3d_pycg_cpu*.whl ./
mv whl_packages/out/* ./
rm -rf whl_packages/tmp/
```

**Making Package Available Online**

```bash
# Append record given current wheels.
python whl_packages/create_index.py
# Upload index.html
awsm cp whl_packages/index.html s3://pycg/packages/ $AWSGA
# Upload wheels
find whl_packages/ -name '*.whl' | while read file; do awsm cp $file s3://pycg/packages/ $AWSGA; done
```

~~I tried to name my mod `open3d-pycg` in PyPI, but failed (PEP 440 doesn't allow git revision as version number + 100MB file upload limit). To make pip auto determine version, I set up my own PyPI using the tutorial from [here](https://testdriven.io/blog/private-pypi/).~~
1. ~~Upload the artifact to `eagle` under `/root/pypi/packages`~~
2. ~~Install via `pip install -U --index-url http://eagle.huangjh.tech:8080 --trusted-host eagle.huangjh.tech open3d`~~

~~The CI/CD workflow will be automatically executed once pushed. I delete something from `.github/workflow` to make the display more concentrated.~~ (The GH CI/CD is completely disabled because it took too much space and my quota...)


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
7. Add `FilamentScene::GetSunLightDirection` python binding.
8. Add `PointCloud::OrientNormalsFloodFill` for consistent normal orientation (from MeshLab).
9. Add a `gui.Histogram` widget type to display data histogram of a specific geometry. Open3D implements its own system to control the layouts of imgui components (note IMGUI is not being controlled! It is just instructed to follow the spawn location Open3D assigned), that does not support floating windows. We escape that stupid thing and build our own nice histogram window in `cpp/open3d/visualization/gui/Histogram.cpp`.
10. Migrate imgui from 1.74 to 1.86 version, add implot dependency to draw histogram.
11. Due to a bug in implmenetation, we have to add `gui.Histogram` before the settings panel widget! So we change the constructor of `gui.O3DVisualizer` to accomplish that.
12. Merged with upstream 0.15. Replace the incoming `window.h`'s setKeyEvent to setKey as originally defined.
13. Add `o3dvisualizer.enable_sun_follows_camera` binding.
14. Merged with upstream 0.15.2.
15. Add `gui.Keyframer` to support animation editing.
16. Enable backface culling binding (double-sided) in filament.
17. Enable selecting model and edit its pose in the new viewer.
18. Add `set_indirect_light_rotation` binding.
