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

Remaining remote work:

None. D01-D05 are complete.

D06 is the first unavoidable Unreal-workstation gate. Work should not begin on
D07-D14 until the dependencies listed on each card are complete.

## WIP Recommendation

Keep no more than two cards in `Now`. No remote foundation work remains before
D06; begin the clean Unreal project shell at the workstation. D07 can follow as
soon as the project shell is healthy because its approved audio is now ready.
