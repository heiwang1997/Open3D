from zipfile import ZipFile, ZIP_DEFLATED
from argparse import ArgumentParser
from pathlib import Path
from tqdm import tqdm


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument('path', type=str, help='Directory to search for wheels')
    parser.add_argument('target_version', type=str, help='x.x.x.post1')
    args = parser.parse_args()

    base_path = Path(args.path)
    whl_paths = base_path.glob("./*.whl")
    whl_paths = list(whl_paths)

    source_version = whl_paths[0].stem.split('-')[1]
    target_version = args.target_version
    print("Source version is", source_version)

    (base_path / "out").mkdir(exist_ok=True, parents=True)
    for whl_path in whl_paths:
        whl_out_path = base_path / "out" / whl_path.name.replace(source_version, target_version)
        with ZipFile(whl_path, "r") as in_f:
            with ZipFile(whl_out_path, "w", ZIP_DEFLATED) as out_f:
                for in_name in tqdm(in_f.namelist()):
                    out_name = in_name.replace(source_version, target_version)
                    in_data = in_f.read(in_name)
                    if in_name.endswith(".py") or in_name.endswith("METADATA") or in_name.endswith("RECORD"):
                        in_data = in_data.replace(
                            source_version.encode('utf-8'), target_version.encode('utf-8'))
                    out_f.writestr(out_name, in_data)
