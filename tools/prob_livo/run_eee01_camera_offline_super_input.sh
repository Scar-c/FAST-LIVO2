#!/usr/bin/env bash
# Prompt-4 diagnostic wrapper: same in-process FAST/LIVO2 offline runner with
# the explicitly selected Super NTU input semantics.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export PROB_LIVO_RUN_ID="${PROB_LIVO_RUN_ID:-eee01_camera_offline_super_input}"
export PROB_LIVO_INPUT_SEMANTICS="super_ntu_legacy"
export PROB_LIVO_CONFIG_OVERLAY="${PROB_LIVO_CONFIG_OVERLAY:-$SCRIPT_DIR/../../config/prob_livo/NTU_eee01_super_legacy.yaml}"
exec "$SCRIPT_DIR/run_eee01_camera_offline.sh" "$@"
