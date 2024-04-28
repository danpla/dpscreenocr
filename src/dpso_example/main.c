
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "dpso/dpso.h"
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


static void setupHotkeys(void)
{
    const DpsoHotkey toggleSelectionHotkey = {
        dpsoKeyGrave, dpsoKeyModCtrl
    };

    dpsoKeyManagerSetIsEnabled(true);

    dpsoKeyManagerBindHotkey(
        &toggleSelectionHotkey, hotkeyActionToggleSelection);

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
    DpsoOcrJobResults results;
    dpsoOcrFetchResults(ocr, &results);

    for (int i = 0; i < results.numItems; ++i) {
        const DpsoOcrJobResult* result = &results.items[i];
        printf(
            "=== %s ===\n%s\n",
            result->timestamp, result->text);
    }
}


static void checkHotkeyActions(DpsoOcr* ocr)
{
    const DpsoHotkeyAction hotkeyAction =
        dpsoKeyManagerGetLastHotkeyAction();
    if (hotkeyAction != hotkeyActionToggleSelection)
        return;

    if (!dpsoSelectionGetIsEnabled()) {
        dpsoSelectionSetIsEnabled(true);
        return;
    }

    dpsoSelectionSetIsEnabled(false);

    DpsoOcrJobArgs jobArgs;
    dpsoSelectionGetGeometry(&jobArgs.screenRect);
    if (dpsoRectIsEmpty(&jobArgs.screenRect))
        return;

    jobArgs.flags = dpsoOcrJobTextSegmentation;

    if (!dpsoOcrQueueJob(ocr, &jobArgs))
        fprintf(stderr, "dpsoQueueJob() error: %s\n", dpsoGetError());
}


volatile sig_atomic_t interrupted;


static void sigintHandler(int signum)
{
    (void)signum;
    interrupted = 1;
}


int main(void)
{
    if (!dpsoInit()) {
        fprintf(stderr, "dpsoInit() error: %s\n", dpsoGetError());
        return EXIT_FAILURE;
    }

    if (dpsoOcrGetNumEngines() == 0) {
        fprintf(stderr, "No OCR engines available\n");
        dpsoShutdown();
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
        dpsoShutdown();
        return EXIT_FAILURE;
    }

    printf(
        "Using %s OCR engine, version %s\n",
        ocrEngineInfo.name,
        ocrEngineInfo.version);

    setupLanguages(ocr);
    setupHotkeys();

    DpsoOcrProgress lastProgress = {0};

    signal(SIGINT, sigintHandler);
    while (!interrupted) {
        dpsoUpdate();

        reportProgress(ocr, &lastProgress);
        checkResults(ocr);
        checkHotkeyActions(ocr);

        dpsoSleep(1000 / 60);
    }

    dpsoOcrDelete(ocr);
    dpsoShutdown();

    return EXIT_SUCCESS;
}
