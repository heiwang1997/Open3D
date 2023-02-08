import re
from pathlib import Path

src_folder = Path(__file__).parent

with (src_folder / "index.html").open("r") as f:
    content = f.read()

origin_data = re.findall(r'href=\".*?\">(.*?)</a>', content)
origin_files = set(origin_data)

current_files = src_folder.glob("*.whl")
current_files = [t.name for t in current_files]
current_files = set(current_files)

all_files = current_files.union(origin_files)
all_files = sorted(list(all_files))

with (src_folder / "index.html").open("w") as f:
    for fn in all_files:
        fn_html = fn.replace('+', '%2B')
        f.write(f'<a href="https://pycg.s3.ap-northeast-1.amazonaws.com/packages/{fn_html}">{fn}</a><br>\n')
