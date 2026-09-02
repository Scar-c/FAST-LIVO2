#!/usr/bin/env bash
# Dedicated Prompt-4 canonical runner: FAST host + Super NTU input semantics.
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export PROB_LIVO_RUN_ID="${PROB_LIVO_RUN_ID:-eee01_camera_off_p0_p4_super_input_parity}"
export PROB_LIVO_INPUT_SEMANTICS="super_ntu_legacy"
export PROB_LIVO_CONFIG_OVERLAY="${PROB_LIVO_CONFIG_OVERLAY:-$SCRIPT_DIR/../../config/prob_livo/NTU_eee01_super_legacy.yaml}"
exec "$SCRIPT_DIR/run_eee01_camera_off.sh" "$@"
