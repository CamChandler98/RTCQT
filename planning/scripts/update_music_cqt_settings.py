"""Edit DEFAULTS below, then run with Unreal's Tools > Execute Python Script.

Applies every CQT option to the catalog's MusicLibrary analyzer pairs through
the rerunnable importer. Wait for 'Music library complete' in the Output Log.
Run while PIE is stopped. Future imports retain these library defaults.
"""
from pathlib import Path
import unreal

# EDIT THESE VALUES. These defaults match the verified baseline in your screenshot.
DEFAULTS = {
    "starting_frequency": 40.0,       # Hz: 20 .. 20000
    "num_bands": 32,                  # 1 .. 96; Music Lab rebuilds its bars automatically
    "num_bands_per_octave": 3.5,       # 1 .. 24
    "analysis_period": 0.01,          # seconds: 0.01 .. 1.0
    "downmix_to_mono": True,          # False analyzes channels separately; Lab queries channel 0
    "fft_size": "XX_LARGE",           # MIN=64, XX_SMALL=128, X_SMALL=256, SMALL=512,
                                     # MEDIUM=1024, LARGE=2048, X_LARGE=4096, XX_LARGE=8192, MAX=16384
    "window_type": "BLACKMAN",        # NONE, HAMMING, HANN, BLACKMAN
    "spectrum_type": "POWER_SPECTRUM", # MAGNITUDE_SPECTRUM or POWER_SPECTRUM
    "band_width_stretch": 1.0,        # 0.01 .. 2.0
    "cqt_normalization": "EQUAL_ENERGY", # EQUAL_EUCLIDEAN_NORM, EQUAL_ENERGY, EQUAL_AMPLITUDE
    "noise_floor_db": -60.0,          # -120 .. 0
}

assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor(), "Stop Play before updating CQT"
root = Path(unreal.Paths.project_dir()).resolve()
script = root / "planning/scripts/import_music_library.py"
exec(compile(script.read_text(encoding="utf-8"), str(script), "exec"),
     {"__name__": "music_cqt_batch", "CQT_OVERRIDE": DEFAULTS})
