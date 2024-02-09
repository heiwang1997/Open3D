from argparse import ArgumentParser
from pathlib import Path
import shutil
from wheel.wheelfile import WheelFile
from wheel.cli.pack import pack


def refresh_imports(path: Path):
    """ Change all imports of open3d into open3d_pycg. Also remove some imports that are not needed for visualization."""
    with path.open("r") as f:
        content = f.read()
    content = content.replace("import open3d.ml", "")
    content = content.replace("import open3d", "import open3d_pycg")
    content = content.replace("from ._external_visualizer import *", "")
    content = content.replace("from .draw_plotly import draw_plotly", "")
    content = content.replace("_server", "")
    content = content.replace("from .to_mitsuba import to_mitsuba", "")
    content = content.replace("open3d.", "open3d_pycg.")
    with path.open("w") as f:
        f.write(content)


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument('path', type=str, help='Directory to search for wheels')
    args = parser.parse_args()

    # Iterate all wheels in the directory
    base_path = Path(args.path)
    whl_paths = base_path.glob("./*.whl")
    whl_paths = list(whl_paths)

    (whl_out_path := base_path / "out").mkdir(exist_ok=True, parents=True)
    for whl_path in whl_paths:
        tmp_path = base_path / "out" / "tmp"
        shutil.rmtree(tmp_path, ignore_errors=True)

        # Extract wheel contents
        with WheelFile(whl_path) as wf:
            wf.extractall(tmp_path)

        base_folders = list(tmp_path.glob("*"))
        dist_folder = [x for x in base_folders if "dist-info" in x.name][0]

        # Massage dist-folder
        (dist_folder / "entry_points.txt").unlink(missing_ok=True)
        with (dist_folder / "top_level.txt").open("w") as f:
            f.write("open3d_pycg")

        # Massage metadata (remove useless dependencies)
        with (dist_folder / "METADATA").open("r") as f:
            content = f.read()
            lines = content.split("\n")
            for i, line in enumerate(lines):
                if "werkzeug" in line or "dash" in line or "nbformat" in line or "configargparse" in line:
                    lines[i] = ""
            content = "\n".join(lines)
        with (dist_folder / "METADATA").open("w") as f:
            f.write(content)

        # Massage src folder
        src_folder = tmp_path / "open3d_pycg"
        if (tmp := tmp_path / "open3d").exists():
            tmp.rename(src_folder)
        
        # Remove useless files and folders
        (src_folder / "app.py").unlink(missing_ok=True)
        (src_folder / "web_visualizer.py").unlink(missing_ok=True)
        shutil.rmtree(src_folder / "examples", ignore_errors=True)
        shutil.rmtree(src_folder / "ml", ignore_errors=True)
        shutil.rmtree(src_folder / "tools", ignore_errors=True)
        shutil.rmtree(src_folder / "visualization" / "tensorboard_plugin", ignore_errors=True)
        (src_folder / "visualization" / "async_event_loop.py").unlink(missing_ok=True)
        (src_folder / "visualization" / "draw_plotly.py").unlink(missing_ok=True)
        (src_folder / "visualization" / "__main__.py").unlink(missing_ok=True)
        (src_folder / "visualization" / "_external_visualizer.py").unlink(missing_ok=True)
        (src_folder / "visualization" / "to_mitsuba.py").unlink(missing_ok=True)

        refresh_imports(src_folder / "__init__.py")
        refresh_imports(src_folder / "visualization" / "__init__.py")

        # Re-pack into wheels (Hash will be recomputed)
        pack(tmp_path, whl_out_path, None)
