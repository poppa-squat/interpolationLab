# Interpolation Lab

A GPL-3 VST3 (and Standalone) test bed for interpolating **Source** toward **Target** with a **Mix** knob. Methods register in a catalog. The plugin UI exposes the selected method as an **Interpolator** dropdown.

1. **Fade** — time-domain equal-power mix.
2. **Spectral Transport** — Henderson and Solomon 2019 portamento from [audioTransport](https://github.com/poppa-squat/audioTransport). Pitches slide instead of merely fading.
3. **RAVE Latent** — encode each live bus with a streaming [RAVE](https://github.com/acids-ircam/RAVE) autoencoder, mix the latent frames with Mix as `{1 - mix, mix}`, decode. This is the smallest reconstructable-embedding interpolator; CLAP-style semantic embeddings still need a generative decoder and are out of scope here.

The interpolator seam takes **N sources and N weights**, not a hard-coded pair. The plugin currently wires two buses and maps Mix to `{1 - mix, mix}` so a DAW can host experiments today. More sources can be added later without changing method implementations.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

JUCE copies the VST3 to `~/.vst3` after a successful plugin build (`COPY_PLUGIN_AFTER_BUILD`).

Default builds stay torch-free. Catch2 covers the barycentric mix and a dummy invertible autoencoder. To load a real streaming RAVE model at runtime:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DINTERPOLATION_LAB_USE_TORCH=ON
cmake --build build -j
export INTERPOLATION_LAB_RAVE_MODEL=/path/to/streaming_model.ts
```

Export the checkpoint with `rave export --run /path/to/run --streaming` so cached convolutions are realtime-safe. Point `LIBTORCH_ROOT` at an existing LibTorch tree if you do not want CMake to download the CPU zip. Typical IRCAM weights are CC-BY-NC and are **not** bundled with this GPL-3 plugin. Without a model (or without LibTorch) **RAVE Latent** outputs silence.

On Linux, add LibTorch's `lib` directory to `LD_LIBRARY_PATH` so the DAW can load the plugin.

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
6. Switch Interpolator to **Spectral Transport** and repeat: Mix = 0 is still this track; Mix = 1 is still the sidechain; Mix in the middle on two steady tones slides pitch instead of fading. Plugin delay compensation may jump; that is expected.
7. Switch Interpolator to **RAVE Latent** with `INTERPOLATION_LAB_RAVE_MODEL` set to a streaming `.ts` file: Mix = 0 is still this track; Mix = 1 is still the sidechain; Mix in the middle should timbre-morph rather than equal-power fade. Delay compensation may jump again.
8. RAVE Latent with no model file: output is silence at every Mix. That is the missing-model signal, not a dry fade.
9. Stereo Source with a mono Target: both output channels morph toward that Target.
10. Disconnect Target, Mix above 0: output heads toward silence.

Standalone builds with the plugin. The Target bus is often silent there unless the audio device exposes extra inputs; use a DAW to experiment with two live sources.
