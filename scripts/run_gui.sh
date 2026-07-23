#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
project_root=$(cd -- "${script_dir}/.." && pwd)
cli_path="${project_root}/build/apps/ballistic_cli/ballistics_cli"
python_path="${project_root}/.venv/bin/python"

cd "${project_root}"
if [[ ! -x "${cli_path}" ]]; then
    "${project_root}/scripts/build_debug.sh"
fi
if [[ ! -x "${python_path}" ]]; then
    echo "ballistics_gui: run ./scripts/setup_gui.sh first" >&2
    exit 2
fi
if [[ -z "${DISPLAY:-}" ]]; then
    echo "ballistics_gui: an X11 display is required for the embedded Panda3D window" >&2
    exit 2
fi
exec env QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"     "${python_path}" -m apps.ballistics_gui.main --cli "${cli_path}" "$@"
