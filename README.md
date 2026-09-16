# Interpolation Lab

A GPL-3 VST3 (and Standalone) test bed for interpolating **Source** toward **Target** with a **Mix** knob. Methods register in a catalog; the first method is **Fade**, a time-domain equal-power mix.

The interpolator seam takes **N sources and N weights**, not a hard-coded pair. The plugin currently wires two buses and maps Mix to `{1 - mix, mix}` so a DAW can host experiments today. More sources can be added later without changing method implementations.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

JUCE copies the VST3 to `~/.vst3` after a successful plugin build (`COPY_PLUGIN_AFTER_BUILD`).

## Adding a method

1. Implement `interpolation_lab::Interpolator` under `src/dsp/methods/`.
2. Append `{ id, name, factory }` at the **end** of the list in `src/dsp/InterpolatorCatalog.cpp` (do not reorder).
3. Add the `.cpp` to `interpolation_lab_dsp` in `CMakeLists.txt`.
4. Add a Catch2 file under `tests/`.

`process()` receives aligned `ConstAudioView`s and barycentric `weights`. Two-source methods can read Mix as `weights[1]`. Methods own any extra latency, buffering, or worker threads behind that call.

## REAPER checklist

1. Insert **Interpolation Lab** on a track that has audio (this is Source).
2. Route another track into the plugin's **Target** / sidechain pins (REAPER: pin connector on the plugin, or a send onto the track's extra inputs).
3. Interpolator = **Fade**, Mix = 0: output is this track.
4. Mix = 1: output is the sidechain track.
5. Mix in the middle: the sounds blend; pitches stay put.

Standalone builds with the plugin. The Target bus is often silent there unless the audio device exposes extra inputs; use a DAW to experiment with two live sources.
