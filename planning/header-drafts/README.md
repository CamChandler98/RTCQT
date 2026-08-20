# D04 Header Drafts

These files are implementation-ready interface drafts for delivery card D04.
They are intentionally outside an Unreal `Source/` directory and do not compile
as part of the legacy `SynRTCQT` module.

D06 copies them into the clean `Source/RTCQT/` runtime module and verifies the
engine-facing include paths and signatures against the exact installed Unreal
version. Behavioral changes must first update
`planning/documents/unreal_runtime_api_spec.md`.

Draft contents:

- `RTCQTTypes.h`: reflected layout, range, and evaluation types.
- `RTCQTResponseProcessor.h`: plain stateless D02 interface.
- `RTCQTResponseSettings.h`: artist-facing response data asset.
- `RTCQTLayout.h`: plain deterministic D03 interface.
- `RTCQTVisualizer.h`: actor ownership and explicit-time surface.
- `RTCQT.Build.cs`: minimal runtime dependency boundary.
