#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Each cloud task is already isolated. Use this checkout; do not create a worktree.
REELGLUE_TOOLS=/workspace/tooling
if [[ ! -x "$REELGLUE_TOOLS/cmake/data/bin/cmake" || ! -x "$REELGLUE_TOOLS/bin/ninja" ]]; then
    python -m pip install --target "$REELGLUE_TOOLS" cmake==3.31.6 ninja==1.11.1.3
fi
if [[ ! -f "$REELGLUE_TOOLS/sysroot/usr/include/X11/Xlib.h" || ! -x "$REELGLUE_TOOLS/sysroot/usr/bin/Xvfb" ]]; then
    mkdir -p "$REELGLUE_TOOLS/apt/lists/partial" "$REELGLUE_TOOLS/apt/archives/partial" "$REELGLUE_TOOLS/sysroot"
    python - <<'PY'
from pathlib import Path
p=Path('/workspace/tooling/apt')
(p/'sources.list').write_text('deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie main\n')
(p/'apt.conf').write_text('''Dir::Etc::main "-";
Dir::Etc::parts "-";
Dir::Etc::sourcelist "/workspace/tooling/apt/sources.list";
Dir::Etc::sourceparts "-";
Dir::State::lists "/workspace/tooling/apt/lists";
Dir::Cache::archives "/workspace/tooling/apt/archives";
APT::Get::List-Cleanup "0";
Acquire::https::Proxy "http://proxy:8080";
APT::Sandbox::User "agent";
''')
PY
    APT_CONFIG="$REELGLUE_TOOLS/apt/apt.conf" /usr/bin/apt-get update
    APT_CONFIG="$REELGLUE_TOOLS/apt/apt.conf" /usr/bin/apt-get -o Debug::NoLocking=true --download-only --no-install-recommends -y install \
        libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libfreetype-dev libfontconfig-dev libgl-dev xvfb xauth
    python - <<'PY'
from pathlib import Path
import subprocess
root=Path('/workspace/tooling/sysroot')
for p in Path('/workspace/tooling/apt/archives').glob('*.deb'):
    subprocess.run(['dpkg-deb','-x',str(p),str(root)],check=True)
for p in root.rglob('*'):
    if p.is_symlink() and not p.exists():
        relative=p.readlink()
        if not relative.is_absolute():
            target=p.parent/relative
            host=Path('/')/target.relative_to(root)
            if host.exists(): target.symlink_to(host)
PY
fi
source scripts/cloud-env.sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_MAKE_PROGRAM="$REELGLUE_TOOLS/bin/ninja" \
    -DCMAKE_PREFIX_PATH="$REELGLUE_TOOLS/sysroot/usr" -DREELGLUE_BUILD_RENDERER=ON
cmake --build build --parallel 3
ctest --test-dir build --output-on-failure
