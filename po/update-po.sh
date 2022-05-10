#!/bin/sh

APP_NAME='dpScreenOCR'
APP_FILE_NAME='dpscreenocr'
BUGS_ADDRESS='https://github.com/danpla/dpscreenocr/issues'

# gettext supports desktop entries since version 0.19. However, we
# use intltool to make the script work on Ubuntu 14.04, which is
# shipped with gettext 0.18.3.
cp ../data/$APP_FILE_NAME.desktop $APP_FILE_NAME.desktop.in
sed -e '/^Comment\[/ d' \
    -e 's/^Comment/_Comment/' \
    -i $APP_FILE_NAME.desktop.in
intltool-extract --quiet --type=gettext/ini $APP_FILE_NAME.desktop.in

# We include the current directory for the *.desktop.in.h file.
xgettext --files-from=POTFILES.in --from-code=UTF-8 --add-comments \
    --package-name=$APP_NAME \
    --msgid-bugs-address=$BUGS_ADDRESS \
    -D.. -D. --output=$APP_FILE_NAME.pot \
    -k_ -kN_ \
    $APP_FILE_NAME.desktop.in.h

for f in *.po
do
    msgmerge --quiet --update --no-fuzzy-matching --backup=off \
        $f $APP_FILE_NAME.pot
done


intltool-merge --quiet --desktop-style \
    . $APP_FILE_NAME.desktop.in ../data/$APP_FILE_NAME.desktop
rm $APP_FILE_NAME.desktop.in.h $APP_FILE_NAME.desktop.in
