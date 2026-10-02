# D12 band material

`M_RTCQT_Bands` is the original, tracked material used by the offline line
visualizer. It uses one shared dynamic material instance per visualizer
actor and two per-instance floats, with no per-band material objects.

| Input or parameter | Meaning | Baseline |
| --- | --- | --- |
| Custom data 0 | Same D02 response used for height | [0, 1] |
| Custom data 1 | Normalized band index | index / (count - 1); one band uses 0 |
| LowBandColor | Low-index tint, linear RGB | (0.01, 0.65, 1.0), cyan |
| HighBandColor | High-index tint, linear RGB | (0.85, 0.01, 0.65), magenta |
| EmissiveMin | Lower shared shader intensity | 0 |
| EmissiveMax | Upper shared shader intensity | 10 |

Use palette RGB components in [0, 1]. The actor sanitizes each emissive bound:
non-finite becomes 0, finite clamps to [0, 100], and the pair is ordered.
Material scalar defaults provide the [0, 10] preview; actor evaluation reads
the current settings asset. Editing the scalar defaults alone does not
replace the actor-owned Emissive Range settings.

Response and band-index nodes each pass through a VertexInterpolator and
Saturate. Let their outputs be r and b. The graph is:

```
tint      = lerp(LowBandColor, HighBandColor, b)
baseColor = tint * lerp(0.25, 1.0, r)
emissive  = tint * lerp(EmissiveMin, EmissiveMax, r)
```

The material is opaque Default Lit, with metallic 0 and roughness 0.65, and
has Instanced Static Mesh usage enabled. Quiet cubes retain a lit base color
when their emissive intensity is zero. Camera orientation determines which
palette end appears on the left of the screen.

The private test scene uses one unbound Post Process Volume with priority
100 and blend weight 1. Min EV100 and Max EV100 both equal **2**. Exposure
compensation is 0, its curve is unset, and bloom intensity is 0.15. The camera
has no enabled exposure overrides. The existing lighting and framing remain
as authored in D11. This exposure baseline disables global eye adaptation;
Lumen and other temporal rendering effects remain separate from the
stateless material inputs.

Open `Content/Audio/PrivateTest/L_D10D11ResponseTest`, select the existing
visualizer, and inspect its Instance Material assignment. Play invokes the
existing 16.75-second evaluation and shows the strong frame. Open
`Content/Materials/M_RTCQT_Bands` to inspect the graph. The private test level,
settings, analyzer, and licensed sound are intentionally ignored.

`planning/scripts/setup_d12_material_scene.py` reproduces this material and
private scene configuration through the installed Unreal editor APIs. It
only replaces a material carrying its own D12 metadata tag, snapshots old
nodes before deletion, and requires successful compilation of the 17-node
graph. Preserve private level/settings backups before running it.

See `planning/documents/d12_material_exposure_checkpoint.md` for validation.
