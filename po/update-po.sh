#!/bin/sh

APP_NAME='dpScreenOCR'
APP_FILE_NAME='dpscreenocr'
BUGS_ADDRESS='https://github.com/danpla/dpscreenocr/issues'

cp ../data/$APP_FILE_NAME.desktop $APP_FILE_NAME.desktop.in
sed -e '/Comment\[/ d' \
    -e 's/Comment/_Comment/' \
    -i $APP_FILE_NAME.desktop.in
intltool-extract --quiet --type=gettext/ini $APP_FILE_NAME.desktop.in


xgettext --files-from=POTFILES.in --from-code=UTF-8 --add-comments \
    --package-name=$APP_NAME \
    --msgid-bugs-address=$BUGS_ADDRESS \
    -D.. -D. --output=$APP_FILE_NAME.pot \
    -k_ -kN_ -kpgettext_expr:1c,2 -knpgettext_expr:1c,2,3 \
    $APP_FILE_NAME.desktop.in.h

for f in *.po;
do
    [ -e "$f" ] || continue
    echo $f
    msgmerge -q --update --no-fuzzy-matching --backup=off $f $APP_FILE_NAME.pot;
done


intltool-merge --quiet --desktop-style \
    . $APP_FILE_NAME.desktop.in ../data/$APP_FILE_NAME.desktop
rm -f $APP_FILE_NAME.desktop.in.h
rm -f $APP_FILE_NAME.desktop.in
