#!/usr/bin/env python3

# Some details about the external programs used. See --help for a
# short list of them.
#
#
# SVG rendering
# =============
#
# We need to use librsvg to get perfectly rendered SVGs. This can be
# done via rvg-convert, ImageMagick, or Inkscape.
#
# On Windows, we always use ImageMagick: as long as it comes from the
# official site, it has libsrvg built in. There are no official
# rvg-convert binaries for Windows, and we don't want to require
# installing them from MSYS2 (which, by the way, will pull
# dependencies for more megabytes than the size of the official
# ImageMagick binaries).
#
# On Unix-like systems, we always use rvg-convert, as there's no
# guarantee that ImageMagick is build with librsvg (if not, we really
# don't want to use IM's own internal crappy SVG renderer).
#
# On all platforms, Inkscape is the fallback option because it's slow
# to start. We don't want to drop Inkscape: after all, all SVGs are
# drawn with it, so there's a chance it's already installed.
#
#
# ICO creation
# ============
#
# Unfortunately, using the icotool program from the icoutils package
# is not enough to create a proper ICO with embedded PNGs. Icotool can
# read PNGs to store uncompressed images, but embedding is only
# possible with the --raw option that copies the PNG as is.
#
# The catch here is that PNGs inside ICO must be in RGBA format,
# otherwise they will be ignored by Windows. We have to force RGBA,
# because some of our tools (like rsvg-convert and optipng) are smart
# enough not to include the alpha channel if the image doesn't have
# transparent pixels.
#
# ImageMagick >= 6.7.8 automatically uses RGBA for the 256 px image
# when assembling ICO, so we don't need icotool in this case. But for
# older versions, we still have to use icotool, and ImageMagick is
# still needed to force the PNG format to RGBA for the --raw option.

from functools import lru_cache
import argparse
import os
import shutil
import subprocess
import sys
import tempfile


APP_FILE_NAME = 'dpscreenocr'


@lru_cache(maxsize=None)
def get_exe_path(name):
    path = shutil.which(name)
    if not path:
        sys.exit('{} not found'.format(name))

    return path


def create_svg_to_png_converter():
    """Return a function to convert SVG to PNG.

    The returned function will accept 3 arguments: an input SVG path,
    output PNG path, and int denoting the output PNG size.
    """
    if os.name == 'nt':
        magick_exe = shutil.which('magick')
        if magick_exe:
            print('Using ImageMagick for SVG conversion')

            def fn(svg_path, png_path, size):
                subprocess.check_call((
                    magick_exe,
                    '-size', '{0}x{0}'.format(size),
                    svg_path,
                    png_path))

            return fn
    else:
        rsvg_convert_exe = shutil.which('rsvg-convert')
        if rsvg_convert_exe:
            print('Using rsvg-convert for SVG conversion')

            def fn(svg_path, png_path, size):
                subprocess.check_call((
                    rsvg_convert_exe,
                    svg_path,
                    '--height=' + str(size),
                    '--width=' + str(size),
                    '--keep-aspect-ratio',
                    '--format=png',
                    '--output=' + png_path))

            return fn

    inkscape_exe = get_exe_path('inkscape')
    print('Using Inkscape for SVG conversion')

    # Newer versions of Inkscape actually accept the legacy pre-1.0
    # options, but we still try to use the right ones to avoid
    # deprecation messages. See:
    # https://wiki.inkscape.org/wiki/Using_the_Command_Line
    version_info = subprocess.check_output(
        (inkscape_exe, '--version'), universal_newlines=True)

    # The version info string is in form "Inkscape x.y.z (rev)".
    version_str = version_info.split(maxsplit=3)[1]
    version = tuple(int(n) for n in version_str.split('.'))
    using_legacy_opts = version < (1,)

    def fn(svg_path, png_path, size):
        args = [
            inkscape_exe,
            svg_path,
            '--export-height=' + str(size),
            '--export-width=' + str(size),
        ]

        if using_legacy_opts:
            args.append('--export-png=' + png_path)
        else:
            args.append('--export-filename=' + png_path)

        subprocess.check_call(args)

    return fn


def get_svg_path(icon_name, size):
    svg_name = icon_name + '.svg'

    svg_path = os.path.join('sizes', 'scalable', svg_name)
    # The generic SVG file must exist even if there's an SVG for a
    # specific size.
    if not os.path.isfile(svg_path):
        sys.exit('{} does not exist'.format(svg_path))

    svg_size_path = os.path.join('sizes', str(size), svg_name)
    if os.path.isfile(svg_size_path):
        svg_path = svg_size_path

    return svg_path


def gen_png(icon_name, png_path, size):
    if not hasattr(gen_png, '_svg_to_png'):
        gen_png._svg_to_png = create_svg_to_png_converter()

    optipng_exe = get_exe_path('optipng')

    gen_png._svg_to_png(get_svg_path(icon_name, size), png_path, size)

    subprocess.check_call((optipng_exe, '-quiet', png_path))


def get_image_magick_version(image_magick_exe):
    version_info = subprocess.check_output(
        (image_magick_exe, '-version'), universal_newlines=True)

    for line in version_info.splitlines():
        parts = line.split()
        if (len(parts) < 3
                or parts[0] != 'Version:'
                or parts[1] != 'ImageMagick'):
            continue

        # The version string looks like "6.9.10-23".
        return tuple(
            int(n) for n in parts[2].replace('-', '.').split('.'))

    sys.exit(
        'Can\'t determine the version of "{}"'.format(
            image_magick_exe))


def create_ico_generator():
    """Return a function to create an ICO from a set of PNGs.

    The returned function accepts 2 arguments: a mapping from PNG size
    to PNG path, and the output ICO path.
    """
    # ImageMagick >= 7.
    image_magick_exe = shutil.which('magick')

    # ImageMagick < 7; mostly relevant for Unix-like systems. Don't
    # even try "convert" on Windows, since that's the name of the
    # standard Windows utility.
    if not image_magick_exe and os.name != 'nt':
        image_magick_exe = shutil.which('convert')

    if not image_magick_exe:
        sys.exit('ImageMagick not found')

    icotool_exe = None

    # ImageMagick < 6.7.8 doesn't store a 256 px image as a PNG when
    # creating an ICO, so use icotool instead.
    image_magick_version = get_image_magick_version(image_magick_exe)
    if image_magick_version < (6, 7, 8):
        print(
            'Using icotool for ICO creation since the "{}" version '
            '({}) is less than 6.7.8'.format(
                image_magick_exe,
                '.'.join(str(n) for n in image_magick_version)))

        icotool_exe = get_exe_path('icotool')

        def base_fn(png_paths, ico_path):
            args = [icotool_exe, '--create', '--output=' + ico_path]

            png_path_256 = png_paths.get(256)
            if png_path_256:
                args.append('--raw=' + png_path_256)

            args.extend(
                sorted(v for k, v in png_paths.items() if k != 256))

            subprocess.check_call(args)
    else:
        print(
            'Using ImageMagick ({}) for ICO creation'.format(
                image_magick_exe))

        def base_fn(png_paths, ico_path):
            args = [image_magick_exe]
            args.extend(sorted(png_paths.values()))
            args.append(ico_path)

            subprocess.check_call(args)

    # Wrap base_fn in a function that forces RGBA for the 256 px PNG.
    # At the time of writing, we have to do this unconditionally:
    #
    #   * The icotool's --raw option doesn't force RGBA.
    #
    #   * ImageMagick tries to force RGBA, but it was broken from the
    #     start and remains so as of version 7.1.1-20:
    #     https://github.com/ImageMagick/ImageMagick/issues/2756

    png_path_256_rgba = os.path.join(
        tempfile.gettempdir(), 'icon-tool-ico-tmp-256-rgba.png')

    def fn(png_paths, ico_path):
        png_path_256 = png_paths.get(256)
        if not png_path_256:
            base_fn(png_paths, ico_path)
            return

        subprocess.check_call((
            image_magick_exe,
            png_path_256,
            'PNG32:' + png_path_256_rgba))

        png_paths_copy = png_paths.copy()
        png_paths_copy[256] = png_path_256_rgba

        base_fn(png_paths_copy, ico_path)

        os.remove(png_path_256_rgba)

    return fn


def gen_ico(icon_name, out_dir, sizes):
    if not hasattr(gen_ico, '_gen_ico'):
        gen_ico._gen_ico = create_ico_generator()

    os.makedirs(out_dir, exist_ok=True)

    png_tmp_dir = os.path.join(
        tempfile.gettempdir(), 'icon-tool-ico-tmp')
    os.makedirs(png_tmp_dir, exist_ok=True)

    png_paths = {}  # size : path

    for size in sizes:
        png_path = os.path.join(
            png_tmp_dir, '{}-{}.png'.format(icon_name, size))
        gen_png(icon_name, png_path, size)
        png_paths[size] = png_path

    gen_ico._gen_ico(
        png_paths, os.path.join(out_dir, icon_name + '.ico'))

    shutil.rmtree(png_tmp_dir)


def get_icon_names():
    result = []

    for name in os.listdir(os.path.join('sizes', 'scalable')):
        stem, ext = os.path.splitext(name)
        if ext == '.svg':
            result.append(stem)

    return result


def get_icon_sizes():
    return tuple(
        int(n) for n in os.listdir('sizes') if n.isdecimal())


def cmd_gen_png(icon_names, force_overwrite):
    all_icon_names = get_icon_names()

    if 'all' in icon_names:
        icon_names = all_icon_names
    else:
        unknown_icon_names = set(icon_names).difference(
            all_icon_names)
        if unknown_icon_names:
            sys.exit(
                'Unknown icon names: {}'.format(
                    ', '.join(sorted(unknown_icon_names))))

    for icon_name in sorted(icon_names):
        for size in sorted(get_icon_sizes()):
            png_path = os.path.join(
                'sizes', str(size), icon_name + '.png')
            if force_overwrite or not os.path.exists(png_path):
                gen_png(icon_name, png_path, size)


def cmd_remove_outdated_png():
    icon_names = set(get_icon_names())

    for size in get_icon_sizes():
        size_dir_path = os.path.join('sizes', str(size))

        for name in os.listdir(size_dir_path):
            stem, ext = os.path.splitext(name)
            if ext == '.png' and stem not in icon_names:
                os.remove(os.path.join(size_dir_path, name))


def cmd_gen_ico():
    # See:
    # https://learn.microsoft.com/en-us/windows/win32/uxguide/vis-icons#size-requirements
    APP_ICON_SIZES = (16, 32, 48, 256)

    gen_ico(APP_FILE_NAME, '.', APP_ICON_SIZES)


EPILOG = """
File layout
===========

The tool works with files from the "sizes" directory. The "scalable"
directory contains SVGs to be used as sources for raster icons.
Directories with names of icon sizes in pixels serve two purposes:

  * They may contain alternative SVGs optimized for small sizes. Each
    such SVG, if any, will only be considered if there is a primary
    SVG in the "scalable" directory.

  * They define sizes and location of the PNGs generated by the
    gen_png command.

Dependencies
============

The tool depends on several programs:
  * Unix-like systems:
    * rsvg-convert or Inkscape
    * optipng
    * The gen_ico command additionally requires:
      * ImageMagick
      * icotool from the icoutils package if ImageMagick < 6.7.8
  * Windows:
    * ImageMagick >= 7. It\'s strongly recommended to use binaries
      from the official website, as other versions (e.g. from MSYS2)
      may be misconfigured, resulting in incorrect SVG rendering and
      other problems.
    * optipng
"""


def main():
    parser = argparse.ArgumentParser(
        description='Icon maintenance tool',
        epilog=EPILOG,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    subparsers = parser.add_subparsers()

    gen_png_parser = subparsers.add_parser(
        'gen_png', help='Generate PNG icons')
    gen_png_parser.add_argument(
        'icon_name',
        nargs='+',
        help=(
            'Icon name to generate PNG for. This is the name of an '
            'SVG image from the "sizes/scalable" directory, without '
            'the ".svg" extension. A special name "all" can be used '
            'to include all names.'))
    gen_png_parser.add_argument(
        '--force-overwrite',
        action='store_true',
        help='Overwrite existing files.')
    gen_png_parser.set_defaults(
        func=lambda args: cmd_gen_png(
            args.icon_name, args.force_overwrite))

    remove_outdated_png_parser = subparsers.add_parser(
        'remove_outdated_png',
        help=(
            'Remove PNGs that no longer have corresponding SVGs in '
            'the "sizes/scalable" directory'))
    remove_outdated_png_parser.set_defaults(
        func=lambda args: cmd_remove_outdated_png())

    gen_ico_parser = subparsers.add_parser(
        'gen_ico',
        help='Generate the application icon in the ICO format')
    gen_ico_parser.set_defaults(func=lambda args: cmd_gen_ico())

    args = parser.parse_args()

    if hasattr(args, 'func'):
        args.func(args)
    else:
        parser.print_help()


if __name__ == '__main__':
    main()
