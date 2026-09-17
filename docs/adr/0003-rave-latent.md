# RAVE latent mix is the third catalog method

Language-semantic embeddings (CLAP) are not invertible: interpolating them needs a generative decoder such as AudioLDM, which is clip-level and far too slow for `Interpolator::process`. Reconstructable autoencoder latents are the smallest path that returns sound from an interpolated vector.

RAVE (Caillon & Esling 2021) is that path. Each live source is encoded to aligned latent frames, mixed barycentrically (`z = Σ w_i z_i`, Mix as `{1 - mix, mix}`), and decoded. Linear mixing matches the catalog weights; spherical interpolation is reserved for unit CLAP vectors later.

The vocoder is lossy, so near Mix = 0 and Mix = 1 the method equal-power crossfades with dry Source or Target delayed by the codec latency. Extra sources stay at weight 0 until an N-source UI exists; they still enter the latent sum if a non-zero weight is supplied.

A user-supplied streaming TorchScript model is loaded from `INTERPOLATION_LAB_RAVE_MODEL` when LibTorch is enabled. Weights are not bundled (typical IRCAM checkpoints are CC-BY-NC; this plugin is GPL-3). Without a model the interpolator still registers and outputs silence with zero extra latency. Tests inject a tiny invertible dummy autoencoder so Catch2 stays torch-free.
