#!/usr/bin/env python3

import glob
import os
import shutil
import subprocess
import sys


APP_NAME = 'dpScreenOCR'
APP_FILE_NAME = 'dpscreenocr'
BUGS_ADDRESS = 'https://github.com/danpla/dpscreenocr/issues'


def check_min_version(tool_path, min_version):
    version_info = subprocess.check_output(
        (tool_path, '-V'), text=True).splitlines()[0]

    # Version info from GNU gettext tools looks like:
    # xgettext (GNU gettext-tools) 0.21.1
    version = version_info.rpartition(' ')[2]

    if any(not s.isdigit() for s in version.split('.')):
        sys.exit(
            'Unexpected {} version info string format \"{}\"'.format(
                tool_path, version_info))

    if version < min_version:
        sys.exit(
            '{} version {} is less than {}'.format(
                tool_path, version, min_version))


def main():
    xgettext_path = shutil.which('xgettext')
    if not xgettext_path:
        sys.exit('xgettext not found')

    # We need xgettext 0.19 for desktop file support. msgmerge can be
    # of any version.
    check_min_version(xgettext_path, "0.19")

    msgmerge_path = shutil.which('msgmerge')
    if not msgmerge_path:
        sys.exit('msgmerge not found')

    subprocess.check_call((
        xgettext_path,
        '--files-from=POTFILES.in',
        '--from-code=UTF-8',
        '--add-comments=Translators:',
        '--package-name=' + APP_NAME,
        '--msgid-bugs-address=' + BUGS_ADDRESS,
        '--directory=..',
        '--output=' + APP_FILE_NAME + '.pot',
        '-k_',
        '-kN_'))

    # Handle the desktop entry separately instead of including it in
    # POTFILES.in. This way we can disable the default keyword list,
    # which includes "Name" that should not be translated.
    subprocess.check_call((
        xgettext_path,
        '--from-code=UTF-8',
        '--omit-header',
        '--join-existing',
        '--directory=..',
        '--output=' + APP_FILE_NAME + '.pot',
        '-k',  # Disable default keywords
        '-kComment',
        os.path.join('data', APP_FILE_NAME + '.desktop')))

    for po_path in glob.glob('*.po'):
        subprocess.check_call((
            msgmerge_path,
            '--quiet',
            '--update',
            '--no-fuzzy-matching',
            '--backup=off',
            po_path,
            APP_FILE_NAME + '.pot'))


if __name__ == '__main__':
    main()
