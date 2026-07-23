#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
project_root=$(cd -- "${script_dir}/.." && pwd)
python_command=${BALLISTICS_PYTHON:-python3}

cd "${project_root}"
if [[ ! -x .venv/bin/python ]]; then
    "${python_command}" -m venv .venv
fi
.venv/bin/python -m pip install --requirement requirements-gui.txt
.venv/bin/python -c 'import PyQt6, panda3d, simplepbr; print("Native GUI environment ready")'
