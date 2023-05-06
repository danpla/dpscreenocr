
#pragma once


class QWidget;


namespace langManager {


class LangList;


QWidget* createLangManagerInstallPage(LangList& langList);
QWidget* createLangManagerUpdatePage(LangList& langList);
QWidget* createLangManagerRemovePage(LangList& langList);


}
