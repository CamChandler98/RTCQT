"""Exercise real asset reuse/reset on an isolated owned copy; no map saves."""
import hashlib
import json
import runpy
import uuid
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
OUT = ROOT / "Saved/Automation/PaletteStep3"
OUT.mkdir(parents=True, exist_ok=True)
module = runpy.run_path(str(ROOT / "planning/scripts/setup_palette_presets.py"), run_name="palette_setup_verification")
setup, rows = module["setup"], module["preset_rows"]
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
protected_paths = [ROOT / "Content/Game/RTCQT/Interactions/BP_RTCQTPulseController.uasset",
                   ROOT / "Content/Audio/PrivateTest/MusicLab/L_MusicLab.umap"]
protected = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in protected_paths}
production_before = setup(report_path=OUT / "setup.json")
production_repeat = setup(report_path=OUT / "setup-rerun.json")
assert production_repeat["reused"] and production_repeat["before_sha256"] == production_repeat["after_sha256"]
test_path = "/Game/Audio/PrivateTest/MusicLab/PaletteSetupVerification/DA_" + uuid.uuid4().hex
created = False
try:
    setup(test_path, report_path=None)
    created = True
    asset = assets.load_asset(test_path)
    original = rows(asset)
    presets = list(asset.get_editor_property("presets"))
    presets[0].set_editor_property("low_color", unreal.LinearColor(0.31, 0.42, 0.53, 1.0))
    presets[0].set_editor_property("display_name", "Artist-edited Neon")
    asset.set_editor_property("presets", presets)
    assert assets.save_loaded_asset(asset)
    edited = rows(asset)
    reuse = setup(test_path, report_path=None)
    assert reuse["presets"] == edited and reuse["before_sha256"] == reuse["after_sha256"]
    reset = setup(test_path, reset_defaults=True, report_path=None)
    assert reset["presets"] == original
    assets.set_metadata_tag(asset, "RTCQT.Owner", "RTCQT.UnownedVerification")
    refused = False
    try:
        setup(test_path, report_path=None)
    except AssertionError:
        refused = True
    assert refused, "Setup must refuse assets it does not own"
    assets.set_metadata_tag(asset, "RTCQT.Owner", module["OWNER"])
    production_after = setup(report_path=OUT / "setup-final.json")
    assert production_repeat["after_sha256"] == production_after["after_sha256"]
    assert protected == {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in protected_paths}
    report = dict(passed=True, production_asset=module["ASSET_PATH"],
                  production_hash=production_after["after_sha256"], reused_without_save=True,
                  artist_colors_and_labels_preserved=True, explicit_reset_restores_defaults=True,
                  unowned_asset_refused=True, protected_asset_hashes=protected,
                  map_loaded_or_saved=False)
    (OUT / "preset-verification.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    unreal.log("RTCQT_PALETTE_PRESETS_VERIFIED")
finally:
    if created:
        assert assets.get_metadata_tag(assets.load_asset(test_path), "RTCQT.Owner") == module["OWNER"]
        assert assets.delete_asset(test_path), "Could not remove the isolated verification asset"
