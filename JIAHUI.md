# Jiahui's fork of Open3D

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

