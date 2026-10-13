"""Rerunnable private Music Lab assets; preserves D13's render scene."""
import hashlib
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = "/Game/Audio/PrivateTest/MusicLab"
LEVEL = DEST + "/L_MusicLab"
OWNER = "RTCQT.MusicLab.v1"
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def owned(name, cls, factory):
    path = DEST + "/" + name
    if assets.does_asset_exist(path):
        value = assets.load_asset(path)
        assert isinstance(value, cls) and assets.get_metadata_tag(value, "RTCQT.Owner") == OWNER, path
        return value
    value = tools.create_asset(name, DEST, cls, factory)
    assert value is not None, path
    assets.set_metadata_tag(value, "RTCQT.Owner", OWNER)
    return value


def setup():
    source = ROOT / "Content/Audio/PrivateTest"
    protected = [source / "L_D13SequenceTest.umap", source / "LS_RTCQT_60s.uasset"]
    before = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
    document = json.loads((source / "MusicLibrary/track_catalog.json").read_text())
    assert document["schema_version"] == 1 and len(document["tracks"]) >= 2
    tracks = []
    for row in document["tracks"]:
        sound, analyzer = unreal.load_asset(row["sound_wave"]), unreal.load_asset(row["analyzer"])
        assert isinstance(sound, unreal.SoundWave) and isinstance(analyzer, unreal.ConstantQNRT)
        assert analyzer.get_editor_property("sound") == sound, row["id"]
        item = unreal.RTCQTMusicTrack()
        for key, value in dict(id=row["id"], label=row["label"], sound_wave=sound,
                               analyzer=analyzer, channel=row["channel"]).items():
            item.set_editor_property(key, value)
        tracks.append(item)
    assert len(set(t.get_editor_property("id") for t in tracks)) == len(tracks)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.RTCQTMusicCatalog)
    catalog = owned("DA_MusicLabCatalog", unreal.RTCQTMusicCatalog, factory)
    catalog.set_editor_property("tracks", tracks)
    bp_factory = unreal.BlueprintFactory()
    bp_factory.set_editor_property("parent_class", unreal.RTCQTMusicLabManager)
    bp = owned("BP_MusicLabManager", unreal.Blueprint, bp_factory)
    widget_factory = unreal.WidgetBlueprintFactory()
    widget_factory.set_editor_property("parent_class", unreal.RTCQTMusicLabControls)
    widget = owned("WBP_MusicLabControls", unreal.WidgetBlueprint, widget_factory)
    for asset in (bp, widget):
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        assert assets.save_loaded_asset(asset)
    if not assets.does_asset_exist(LEVEL):
        assert levels.load_level("/Game/Audio/PrivateTest/L_D13SequenceTest")
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        assert unreal.EditorLoadingAndSavingUtils.save_map(world, LEVEL)
        assert levels.load_level(LEVEL)
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        assets.set_metadata_tag(world, "RTCQT.Owner", OWNER)
        assert levels.save_current_level()
    else:
        assert levels.load_level(LEVEL)
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        assert assets.get_metadata_tag(world, "RTCQT.Owner") == OWNER
    scene = actors.get_all_level_actors()
    for actor in scene:
        if isinstance(actor, unreal.LevelSequenceActor):
            assert actors.destroy_actor(actor)
    # Disconnect any copied BeginPlay graph so the manager alone drives playback
    # and visual time. Only the copied Music Lab Level Blueprint is edited.
    level = levels.get_current_level()
    level_bp = unreal.find_object(None, level.get_path_name() + ".L_MusicLab")
    if level_bp:
        graph = unreal.BlueprintEditorLibrary.find_event_graph(level_bp)
        editor = unreal.BlueprintGraphEditor.get_graph_editor(graph)
        event = editor.find_event_node("ReceiveBeginPlay")
        if event:
            pin = event.find_then_pin()
            for connection in list(pin.list_connected_pins()):
                assert pin.break_single_pin_link(connection)
        unreal.BlueprintEditorLibrary.compile_blueprint(level_bp)
    scene = actors.get_all_level_actors()
    visualizers = [a for a in scene if isinstance(a, unreal.RTCQTVisualizer)]
    cameras = [a for a in scene if isinstance(a, unreal.CameraActor)]
    managers = [a for a in scene if isinstance(a, unreal.RTCQTMusicLabManager)]
    assert len(visualizers) == len(cameras) == 1 and len(managers) <= 1
    response_path = DEST + "/DA_MusicLabResponse"
    if not assets.does_asset_exist(response_path):
        original = visualizers[0].get_editor_property("response_settings")
        response = assets.duplicate_asset(original.get_path_name().split(".")[0], response_path)
        assert response is not None
        assets.set_metadata_tag(response, "RTCQT.Owner", OWNER)
        ranges = []
        for label, low, high in (("Bass", 20, 250), ("Mid", 250, 4000), ("Treble", 4000, 20000)):
            item = unreal.RTCQTFrequencyWeight()
            for key, value in dict(label=label, min_hz=low, max_hz=high, gain=1).items():
                item.set_editor_property(key, value)
            ranges.append(item)
        response.set_editor_property("frequency_weights", ranges)
        assert assets.save_loaded_asset(response)
    else:
        response = assets.load_asset(response_path)
        assert assets.get_metadata_tag(response, "RTCQT.Owner") == OWNER
    visualizers[0].set_editor_property("response_settings", response)
    manager = managers[0] if managers else actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector())
    manager.set_actor_label("Music Lab Manager")
    for key, value in dict(catalog=catalog, visualizer=visualizers[0], demo_camera=cameras[0],
                           controls_class=widget.generated_class(), initial_track_id=tracks[0].get_editor_property("id"),
                           auto_play=True).items():
        manager.set_editor_property(key, value)
    for item in tracks:
        analyzer = item.get_editor_property("analyzer")
        sound = item.get_editor_property("sound_wave")
        assert not sound.get_editor_property("looping")
        assert len(analyzer.get_normalized_channel_constant_q_at_time(
            sound.get_editor_property("duration") / 2, item.get_editor_property("channel"))) == analyzer.get_editor_property("settings").get_editor_property("num_bands")
    visualizers[0].clear_visualization()
    assert assets.save_loaded_asset(catalog)
    assert levels.save_current_level()
    assert before == {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
    report = ROOT / "Saved/Automation/MusicLab/setup.json"
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(dict(level=LEVEL, manager=bp.get_path_name(), widget=widget.get_path_name(),
                                     tracks=len(tracks), sequence_actors=0, d13_hashes=before), indent=2))
    unreal.log("Music Lab setup PASSED")


setup()
