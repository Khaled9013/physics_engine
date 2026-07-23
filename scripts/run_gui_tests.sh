#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
project_root=$(cd -- "${script_dir}/.." && pwd)
python_path="${project_root}/.venv/bin/python"

cd "${project_root}"
if [[ ! -x build/apps/ballistic_cli/ballistics_cli ]]; then
    ./scripts/build_debug.sh
fi
if [[ ! -x "${python_path}" ]]; then
    echo "ballistics_gui: run ./scripts/setup_gui.sh first" >&2
    exit 2
fi
"${python_path}" -m unittest discover -s tests/gui -v
ctest --test-dir build --output-on-failure

if [[ "${1:-}" == "--smoke" ]]; then
    if [[ -z "${DISPLAY:-}" ]]; then
        echo "ballistics_gui: --smoke requires an X11 display" >&2
        exit 2
    fi
    smoke_path="${TMPDIR:-/tmp}/ballistics-gui-smoke.png"
    QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}" "${python_path}"         -m apps.ballistics_gui.main         --smoke-test         --smoke-scope         --screenshot "${smoke_path}"
    test -s "${smoke_path}"
    echo "GPU smoke frame: ${smoke_path}"
fi
