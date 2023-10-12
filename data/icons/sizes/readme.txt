The "scalable" directory contains SVGs to be used as sources for PNGs.
Each SVG may have an alternative optimized for small size, located in
the directory with a name corresponding to its size in pixels.

The build scripts will use images from the "scalable" directory as the
definitive list of available icons, i.e. an alternative small SVG, if
any, will only be considered if there is a primary SVG in "scalable".
