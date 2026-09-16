# Interpolator catalog is the extension point

New interpolation methods register at the end of `InterpolatorCatalog`. They implement `Interpolator::process(sources, weights, output)` so a method never assumes exactly two inputs.

The plugin currently exposes two live buses (Source, Target) and maps Mix onto barycentric weights `{1 - mix, mix}`. Extra buses or file-backed sources can be appended later; Mix then remains a 2-source control until an N-source UI exists. Extra sources stay at weight 0.

Catalog order is append-only so the Interpolator choice parameter keeps stable indices.
