#!/bin/sh

# This script requires gettext >= 0.19 to work with desktop entries.

APP_NAME='dpScreenOCR'
APP_FILE_NAME='dpscreenocr'
BUGS_ADDRESS='https://github.com/danpla/dpscreenocr/issues'

xgettext --files-from=POTFILES.in \
    --from-code=UTF-8 \
    --add-comments='Translators:' \
    --package-name="$APP_NAME" \
    --msgid-bugs-address="$BUGS_ADDRESS" \
    --directory=.. \
    --output="$APP_FILE_NAME.pot" \
    -k_ -kN_

# Extract messages from the desktop entries as separate step instead
# of including it in POTFILES.in. This way we can disable the default
# keyword list, which includes "Name" that should not be translated.
xgettext --from-code=UTF-8 \
    --omit-header \
    --join-existing \
    --directory=.. \
    --output="$APP_FILE_NAME.pot" \
    -k -kComment \
    data/dpscreenocr.desktop

for f in *.po
do
    msgmerge --quiet --update --no-fuzzy-matching --backup=off \
        "$f" "$APP_FILE_NAME.pot"
done
