# Prob-LIVO History

## Prompt 0 / I0 — bootstrap and architecture freeze

FAST-LIVO2 is the host because its public scheduler, scan recombination, camera
epoch handling, and visual estimator are the semantics to preserve. The
Prob-LIO repository remains a clean, read-only implementation oracle so the
canonical Super-native P0–P4 backend can be migrated deliberately. P5 is
excluded because it is experimental/non-canonical in the oracle and is not
part of the frozen integration target.

The camera-OFF Prob-LIO backend baseline precedes camera-ON closure so LIO
replacement and visual preservation can be verified separately. A second
LiDAR map is forbidden: Prob OctVox is the sole LiDAR geometry authority and
FAST-LIVO2 `feat_map` owns only visual patches/VisualPoints. The shared-state
contract is one host-layout x19/P19, not two filters with copied pose.

Prompt 0 created the project-owned documentation namespaces, recorded exact
host/reference identities, audited production source seams, inventoried the
local NTU/OXFORD files, and built the untouched host with the existing VIKIT
dependency overlay. No legacy runtime/evaluator asset was imported. Future
imports follow COPY-ON-DEMAND with source commit/path provenance.
