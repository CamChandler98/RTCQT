# RTCQT Offline Delivery Dependencies

This is the readable companion to
`planning/csv/rtcqt_offline_delivery_import.csv`. The CSV `Blocked By` column
is authoritative for card-level prerequisites.

## Critical Path

```text
D01 Architecture decision
  +--> D02 Response specification --------+
  |                                       |
  +--> D03 Layout specification ----------+--> D04 Unreal API design --> D06 Project shell
  |                                                                            |
  +--> D05 Approved test audio -------------------------------------+           |
                                                                  |           |
                              +-----------------------------------+-----------+
                              |                                   |
                              v                                   v
                    D07 ConstantQNRT asset              D08 Instanced line layout
                              |                                   |
                              +----------------+------------------+
                                               v
                                    D10 Explicit-time query

D02 + D06 --> D09 Response implementation
D09 + D10 --> D11 Scale response --> D12 Material/exposure --> D13 Sequencer scene
D13 --> D14 Repeatability proof --> D15 Synchronized media --> D16 OBS scene
D16 --> D17 Private rehearsal --> D18 Runbook

D14 --> D19 Circle and Fibonacci-sphere polish
```

## Parallel Work Windows

- After D01, D02, D03, and D05 may proceed independently.
- D04 waits for D02 and D03, but does not wait for test audio.
- After D06, D07, D08, and D09 can overlap when their other prerequisites are
  ready.
- D15 can be completed remotely once D14 render output is available.
- D18 can be drafted remotely, but cannot be accepted until D17 provides a
  proven rehearsal configuration.

## Remote Work Remaining Before the Unreal Workstation

Completed remotely:

- [x] D01: offline architecture decision.
- [x] D02: response formulas and 32 validated golden test vectors.
- [x] D03: line, circle, and Fibonacci-sphere contracts with 29 validated
  transforms across 17 cases.
- [x] D04: Unreal runtime API, ownership, failure, source-tree, and module
  dependency contract with reviewed header drafts.
- [x] D05: approved Good Kid instrumental test fixture, permission evidence,
  technical fingerprints, deterministic extraction recipe, and six reference
  timestamps.

## Unreal Workstation Progress

Completed:

- [x] D06: UE 5.8 project shell, canonical runtime module, required plugins,
  Development Editor build, interactive reopen, and fresh-clone build/load.
- [x] D07: imported SoundWave, configured 32-band ConstantQNRT asset, and
  complete raw/normalized readings at all six reference timestamps. Reordered
  and fresh-editor runs match the baseline exactly. See
  [D07 verification](d07_constant_qnrt_verification.md).

- [x] D08: deterministic line layout, one instanced mesh component, automated
  layout/lifecycle checks, and a saved 32-cube scene verified after reopening
  and in Play mode. See [D08 verification](d08_line_layout_verification.md).

- [x] D09: stateless C++ response processing, all 32 D02 reference cases,
  batch and numeric boundary checks, and repeatable results across reordered
  calls. All six D09 and four D08 automation groups pass. See
  [D09 verification](d09_response_verification.md).

- [x] D10: exact-time C++ query, safe validation/recovery, all 192 D07
  reference values, and the saved-level Blueprint call. Manual runs at
  0.75, 44.25, and 16.75 seconds return success with 32 bands; a fresh
  editor session reloads the saved level and returns the same 16.75 result.
  See [D10 checkpoints](d10_timestamp_query_checkpoint.md).

Next:

- D11 is unblocked: both D09 and D10 are complete. Apply the shared response
  to the existing instances without rebuilding unchanged topology.

## WIP Recommendation

D10 is `Done`; D11 remains `Next` until implementation begins.
Keep no more than two cards in `Now`. The next visible milestone is D11:
applying the shared response to the existing instances without rebuilding
topology.
