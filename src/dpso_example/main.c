
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "dpso_ocr/dpso_ocr.h"
#include "dpso_sys/dpso_sys.h"
#include "dpso_utils/dpso_utils.h"


static void setupLanguages(DpsoOcr* ocr)
{
    if (dpsoOcrGetNumLangs(ocr) == 0) {
        fprintf(stderr, "Please install language packs\n");
        exit(EXIT_FAILURE);
    }

    int langIdx =
        dpsoOcrGetLangIdx(ocr, dpsoOcrGetDefaultLangCode(ocr));
    if (langIdx == -1) {
        printf(
            "Default language (%s) is not available; using %s\n",
            dpsoOcrGetDefaultLangCode(ocr),
            dpsoOcrGetLangCode(ocr, 0));
        langIdx = 0;
    }

    dpsoOcrSetLangIsActive(ocr, langIdx, true);
}


enum HotkeyActions {
    hotkeyActionToggleSelection
};


static void setupHotkeys(DpsoKeyManager* keyManager)
{
    const DpsoHotkey toggleSelectionHotkey = {
        dpsoKeyGrave, dpsoKeyModCtrl
    };

    dpsoKeyManagerSetIsEnabled(keyManager, true);

    dpsoKeyManagerBindHotkey(
        keyManager,
        &toggleSelectionHotkey,
        hotkeyActionToggleSelection);

    printf(
        "Press %s to toggle selection\n",
        dpsoHotkeyToString(&toggleSelectionHotkey));
}


static void reportProgress(
    const DpsoOcr* ocr, DpsoOcrProgress* lastProgress)
{
    DpsoOcrProgress progress;
    dpsoOcrGetProgress(ocr, &progress);

    if (progress.totalJobs == 0
            || dpsoOcrProgressEqual(&progress, lastProgress))
        return;

    *lastProgress = progress;

    printf(
        "Job %i/%i progress: %2i%%\n",
        progress.curJob, progress.totalJobs, progress.curJobProgress);
}


static void checkResults(DpsoOcr* ocr)
{
    DpsoOcrJobResult result;
    while (dpsoOcrGetResult(ocr, &result))
        printf("=== %s ===\n%s\n", result.timestamp, result.text);
}


static void checkHotkeyActions(DpsoSys* sys, DpsoOcr* ocr)
{
    const DpsoHotkeyAction hotkeyAction =
        dpsoKeyManagerGetLastHotkeyAction(dpsoSysGetKeyManager(sys));
    if (hotkeyAction != hotkeyActionToggleSelection)
        return;

    DpsoSelection* selection = dpsoSysGetSelection(sys);
    if (!dpsoSelectionGetIsEnabled(selection)) {
        dpsoSelectionSetIsEnabled(selection, true);
        return;
    }

    dpsoSelectionSetIsEnabled(selection, false);

    DpsoRect screenRect;
    dpsoSelectionGetGeometry(selection, &screenRect);
    if (dpsoRectIsEmpty(&screenRect))
        return;

    DpsoImg* screenshot = dpsoTakeScreenshot(sys, &screenRect);
    if (!screenshot) {
        fprintf(stderr, "dpsoTakeScreenshot(): %s\n", dpsoGetError());
        return;
    }

    if (!dpsoOcrQueueJob(
            ocr, &screenshot, dpsoOcrJobTextSegmentation))
        fprintf(stderr, "dpsoQueueJob(): %s\n", dpsoGetError());
}


volatile sig_atomic_t interrupted;


static void sigintHandler(int signum)
{
    (void)signum;
    interrupted = 1;
}


int main(void)
{
    DpsoSys* sys = dpsoSysCreate();
    if (!sys) {
        fprintf(stderr, "dpsoSysCreate(): %s\n", dpsoGetError());
        return EXIT_FAILURE;
    }

    if (dpsoOcrGetNumEngines() == 0) {
        fprintf(stderr, "No OCR engines available\n");
        dpsoSysDelete(sys);
        exit(EXIT_FAILURE);
    }

    const int engineIdx = 0;
    const char* dataDir = "";

    DpsoOcrEngineInfo ocrEngineInfo;
    dpsoOcrGetEngineInfo(engineIdx, &ocrEngineInfo);

    DpsoOcr* ocr = dpsoOcrCreate(engineIdx, dataDir);
    if (!ocr) {
        fprintf(
            stderr,
            "dpsoOcrCreate() failed to create \"%s\" engine: %s\n",
            ocrEngineInfo.id, dpsoGetError());
        dpsoSysDelete(sys);
        return EXIT_FAILURE;
    }

    printf(
        "Using %s OCR engine, version %s\n",
        ocrEngineInfo.name,
        ocrEngineInfo.version);

    setupLanguages(ocr);
    setupHotkeys(dpsoSysGetKeyManager(sys));

    DpsoOcrProgress lastProgress = {0};

    signal(SIGINT, sigintHandler);
    while (!interrupted) {
        dpsoSysUpdate(sys);

        reportProgress(ocr, &lastProgress);
        checkResults(ocr);
        checkHotkeyActions(sys, ocr);

        dpsoSleep(1000 / 60);
    }

    dpsoOcrDelete(ocr);
    dpsoSysDelete(sys);

    return EXIT_SUCCESS;
}
