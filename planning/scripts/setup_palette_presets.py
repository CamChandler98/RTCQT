"""Create/reuse only the Music Lab palette asset; never load or save a map.

Commandlet flag -RTCQTPaletteResetDefaults explicitly restores starter values.
The setup() entry point also supports isolated verification assets under MusicLab.
"""
import hashlib
import json
import math
import re
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
ASSET_PATH = "/Game/Audio/PrivateTest/MusicLab/DA_MusicLabPalettes"
OWNER = "RTCQT.PalettePresets.v1"
OUTPUT = ROOT / "Saved/Automation/PaletteStep3/setup.json"


def preset_rows(asset):
    rows = []
    ids = set()
    for preset in asset.get_editor_property("presets"):
        identifier = str(preset.get_editor_property("id"))
        label = str(preset.get_editor_property("display_name"))
        assert identifier and identifier.casefold() != "none" and identifier.casefold() not in ids, identifier
        assert label.strip(), "Palette display names must not be empty"
        ids.add(identifier.casefold())
        row = {"id": identifier, "display_name": label}
        for field in ("low_color", "high_color"):
            color = preset.get_editor_property(field)
            values = [getattr(color, channel) for channel in ("r", "g", "b", "a")]
            assert all(math.isfinite(value) for value in values), (identifier, field)
            row[field] = values
        rows.append(row)
    assert rows, "Palette preset list must not be empty"
    return rows


def setup(asset_path=ASSET_PATH, reset_defaults=False, report_path=OUTPUT):
    assert asset_path.startswith("/Game/Audio/PrivateTest/MusicLab/"), asset_path
    assert re.fullmatch(r"/Game/[A-Za-z0-9_/]+", asset_path), asset_path
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    filename = ROOT / "Content" / (asset_path.removeprefix("/Game/") + ".uasset")
    before_hash = hashlib.sha256(filename.read_bytes()).hexdigest() if filename.is_file() else None
    existed = assets.does_asset_exist(asset_path)
    if existed:
        asset = assets.load_asset(asset_path)
        assert isinstance(asset, unreal.RTCQTPalettePresets), "Existing palette asset has the wrong class"
        assert assets.get_metadata_tag(asset, "RTCQT.Owner") == OWNER, "Refusing to overwrite an unowned palette asset"
    else:
        folder, name = asset_path.rsplit("/", 1)
        assert assets.does_directory_exist(folder) or assets.make_directory(folder)
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.RTCQTPalettePresets)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.RTCQTPalettePresets, factory)
        assert isinstance(asset, unreal.RTCQTPalettePresets), asset_path
        assets.set_metadata_tag(asset, "RTCQT.Owner", OWNER)
    if reset_defaults:
        defaults = unreal.get_default_object(unreal.RTCQTPalettePresets)
        asset.set_editor_property("presets", list(defaults.get_editor_property("presets")))
    rows = preset_rows(asset)
    if not existed or reset_defaults:
        assert assets.save_loaded_asset(asset), "Could not save palette presets"
    assert filename.is_file(), filename
    after_hash = hashlib.sha256(filename.read_bytes()).hexdigest()
    if existed and not reset_defaults:
        assert before_hash == after_hash, "A reuse-only run changed the saved palette asset"
    report = dict(passed=True, asset=asset_path, owner=OWNER, created=not existed,
                  reused=existed and not reset_defaults, reset_defaults=bool(reset_defaults),
                  before_sha256=before_hash, after_sha256=after_hash, presets=rows,
                  map_loaded_or_saved=False)
    if report_path is not None:
        report_path = Path(report_path)
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    unreal.log("RTCQT_PALETTE_SETUP_PASSED " + asset_path)
    return report


if __name__ == "__main__":
    reset = re.search(r"(?:^|\s)-RTCQTPaletteResetDefaults(?:\s|$)",
                      unreal.SystemLibrary.get_command_line(), re.IGNORECASE) is not None
    setup(reset_defaults=reset)
