#pragma once


class QWidget;


namespace ui::qt::langManager {


class LangList;


QWidget* createLangManagerInstallPage(LangList& langList);
QWidget* createLangManagerUpdatePage(LangList& langList);
QWidget* createLangManagerRemovePage(LangList& langList);


}
