"""Author the D12 material and save its assignment and fixed exposure in the private scene.

Run in the editor with -EnablePlugins=PythonScriptPlugin -ExecutePythonScript=<file>.
Back up the private level/settings before running. This edits only the named
material, its assigned test settings, and the existing test level.
"""

from pathlib import Path
import json
import unreal


MATERIAL_PATH = "/Game/Materials/M_RTCQT_Bands"
LEVEL_PATH = "/Game/Audio/PrivateTest/L_D10D11ResponseTest"
FIXED_EV100 = 2.0
library = unreal.MaterialEditingLibrary
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
directory = Path(unreal.Paths.project_saved_dir()).resolve() / "Automation" / "D12"
directory.mkdir(parents=True, exist_ok=True)


def expression(material, cls, x, y, **properties):
    node = library.create_material_expression(material, cls, x, y)
    assert node is not None
    for key, value in properties.items():
        node.set_editor_property(key, value)
    return node


def connect(source, dest, pin):
    assert library.connect_material_expressions(source, "", dest, pin), (dest.get_class().get_name(), pin, list(library.get_material_expression_input_names(dest)))


def setup():
    assert levels.load_level(LEVEL_PATH)
    scene = actors.get_all_level_actors()
    visualizers = [actor for actor in scene if isinstance(actor, unreal.RTCQTVisualizer)]
    assert len(visualizers) == 1
    actor = visualizers[0]
    settings = actor.get_editor_property("response_settings")
    assert settings is not None and settings.get_editor_property("response_exponent") == 2.0
    report = {"level": LEVEL_PATH, "material": MATERIAL_PATH, "actor": actor.get_path_name(),
              "previous_material": actor.get_editor_property("instance_material").get_path_name(),
              "scene_actors": [{"label": item.get_actor_label(), "class": item.get_class().get_name()}
                               for item in scene]}
    material = assets.load_asset(MATERIAL_PATH) if assets.does_asset_exist(MATERIAL_PATH) else None
    if material is not None:
        assert assets.get_metadata_tag(material, "RTCQT.Delivery") == "D12", "Existing material was not authored by this helper"
        # Snapshot before deleting: the engine's DeleteAll iterates the same
        # collection it removes from and can leave disconnected nodes behind.
        for old_node in list(library.get_material_expressions(material)):
            library.delete_material_expression(material, old_node)
        assert library.get_num_material_expressions(material) == 0
    else:
        assert assets.does_directory_exist("/Game/Materials") or assets.make_directory("/Game/Materials")
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_RTCQT_Bands", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
        assert material is not None
    assets.set_metadata_tag(material, "RTCQT.Delivery", "D12")
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    library.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, True)

    response = expression(material, unreal.MaterialExpressionPerInstanceCustomData, -1400, -400, data_index=0, const_default_value=0.0)
    index = expression(material, unreal.MaterialExpressionPerInstanceCustomData, -1400, 200, data_index=1, const_default_value=0.0)
    response_vertex = expression(material, unreal.MaterialExpressionVertexInterpolator, -1200, -400)
    index_vertex = expression(material, unreal.MaterialExpressionVertexInterpolator, -1200, 200)
    r = expression(material, unreal.MaterialExpressionSaturate, -1000, -400)
    b = expression(material, unreal.MaterialExpressionSaturate, -1000, 200)
    connect(response, response_vertex, "VS")
    connect(index, index_vertex, "VS")
    connect(response_vertex, r, "")
    connect(index_vertex, b, "")
    low = expression(material, unreal.MaterialExpressionVectorParameter, -1000, 0,
                     parameter_name="LowBandColor", default_value=unreal.LinearColor(0.01, 0.65, 1.0, 1.0))
    high = expression(material, unreal.MaterialExpressionVectorParameter, -1000, 400,
                      parameter_name="HighBandColor", default_value=unreal.LinearColor(0.85, 0.01, 0.65, 1.0))
    tint = expression(material, unreal.MaterialExpressionLinearInterpolate, -700, 100)
    connect(low, tint, "A")
    connect(high, tint, "B")
    connect(b, tint, "Alpha")
    brightness = expression(material, unreal.MaterialExpressionLinearInterpolate, -700, -300, const_a=0.25, const_b=1.0)
    connect(r, brightness, "Alpha")
    base = expression(material, unreal.MaterialExpressionMultiply, -400, -200)
    connect(tint, base, "A")
    connect(brightness, base, "B")
    minimum = expression(material, unreal.MaterialExpressionScalarParameter, -1000, -700,
                         parameter_name="EmissiveMin", default_value=0.0)
    maximum = expression(material, unreal.MaterialExpressionScalarParameter, -1000, -900,
                         parameter_name="EmissiveMax", default_value=10.0)
    intensity = expression(material, unreal.MaterialExpressionLinearInterpolate, -700, -600)
    connect(minimum, intensity, "A")
    connect(maximum, intensity, "B")
    connect(r, intensity, "Alpha")
    emissive = expression(material, unreal.MaterialExpressionMultiply, -400, -500)
    connect(tint, emissive, "A")
    connect(intensity, emissive, "B")
    roughness = expression(material, unreal.MaterialExpressionConstant, -400, 200, r=0.65)
    metallic = expression(material, unreal.MaterialExpressionConstant, -400, 400, r=0.0)
    for node, prop in [(base, unreal.MaterialProperty.MP_BASE_COLOR),
                       (emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR),
                       (roughness, unreal.MaterialProperty.MP_ROUGHNESS),
                       (metallic, unreal.MaterialProperty.MP_METALLIC)]:
        assert library.connect_material_property(node, "", prop)
    assert library.get_num_material_expressions(material) == 17
    errors = list(library.recompile_material(material))
    assert not errors, errors
    assert library.has_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    assert assets.save_loaded_asset(material, only_if_is_dirty=False)
    actor.set_editor_property("instance_material", material)
    assert actor.rebuild_instances()
    settings.set_editor_property("emissive_range", unreal.RTCQTScalarRange(min=0.0, max=10.0))
    assert assets.save_loaded_asset(settings, only_if_is_dirty=False)

    volumes = [item for item in scene if isinstance(item, unreal.PostProcessVolume)]
    assert len(volumes) <= 1, "Inspect multiple exposure volumes before configuring the baseline"
    volume = volumes[0] if volumes else actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector())
    volume.set_actor_label("D12 - Fixed Exposure")
    volume.set_editor_property("unbound", True)
    volume.set_editor_property("enabled", True)
    volume.set_editor_property("blend_weight", 1.0)
    volume.set_editor_property("priority", 100.0)
    post = volume.get_editor_property("settings")
    for name, value in {"override_auto_exposure_method": True,
                        "auto_exposure_method": unreal.AutoExposureMethod.AEM_HISTOGRAM,
                        "override_auto_exposure_min_brightness": True,
                        "override_auto_exposure_max_brightness": True,
                        "auto_exposure_min_brightness": FIXED_EV100,
                        "auto_exposure_max_brightness": FIXED_EV100,
                        "override_auto_exposure_bias": True, "auto_exposure_bias": 0.0,
                        "override_auto_exposure_bias_curve": True, "auto_exposure_bias_curve": None,
                        "override_bloom_intensity": True, "bloom_intensity": 0.15}.items():
        post.set_editor_property(name, value)
    volume.set_editor_property("settings", post)
    cameras = [item for item in scene if isinstance(item, unreal.CameraActor)]
    report["camera_post_process_blend_weights"] = []
    report["camera_exposure_overrides_disabled"] = []
    for camera in cameras:
        component = camera.get_component_by_class(unreal.CameraComponent)
        weight = component.get_editor_property("post_process_blend_weight")
        report["camera_post_process_blend_weights"].append(float(weight))
        camera_post = component.get_editor_property("post_process_settings")
        changed = []
        for flag in ["override_auto_exposure_method", "override_auto_exposure_min_brightness",
                     "override_auto_exposure_max_brightness", "override_auto_exposure_bias",
                     "override_auto_exposure_bias_curve", "override_auto_exposure_apply_physical_camera_exposure"]:
            if camera_post.get_editor_property(flag):
                changed.append(flag)
                camera_post.set_editor_property(flag, False)
        if changed:
            component.set_editor_property("post_process_settings", camera_post)
        report["camera_exposure_overrides_disabled"].append(changed)
    assert levels.save_current_level()
    report.update({"material_compile_errors": errors, "instanced_mesh_usage": True,
                   "material_expression_count": library.get_num_material_expressions(material),
                   "emissive_range": [0.0, 10.0], "response_exponent": 2.0,
                   "min_ev100": FIXED_EV100, "max_ev100": FIXED_EV100, "exposure_compensation": 0.0,
                   "bloom_intensity": 0.15, "level_saved": True})
    (directory / "material-scene-setup.json").write_text(json.dumps(report, indent=2) + "\n")
    unreal.log("D12_MATERIAL_SCENE_SAVED " + json.dumps(report))


try:
    setup()
except Exception as error:
    unreal.log_error("D12_SETUP_FAILED " + str(error))
    raise
finally:
    unreal.SystemLibrary.quit_editor()
