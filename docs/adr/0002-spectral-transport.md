# Spectral Transport is the second catalog method

Spectral Transport is Henderson and Solomon 2019 portamento, ported from audioTransport into this catalog. It still interpolates the first two sources using Mix as `weights[1]`; extra sources stay at weight 0 until an N-source UI exists.

The vocoder always runs so phase state stays warm. Near Mix = 0 and Mix = 1 it equal-power crossfades with dry Source or Target delayed by the vocoder latency so the knob extremes are the original sounds. Fade remains a mutually exclusive test contrast and is not delay-matched, so switching Interpolator may jump host plugin delay compensation.
