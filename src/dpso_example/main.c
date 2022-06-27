
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dpso/dpso.h"


static void setupLanguages(DpsoOcr* ocr)
{
    int langIdx;

    if (dpsoOcrGetNumLangs(ocr) == 0) {
        fprintf(stderr, "Please install language packs\n");
        exit(EXIT_FAILURE);
    }

    langIdx = dpsoOcrGetLangIdx(ocr, dpsoOcrGetDefaultLangCode(ocr));
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

    dpsoSetHotheysEnabled(true);

    dpsoBindHotkey(
        &toggleSelectionHotkey, hotkeyActionToggleSelection);

    printf(
        "Press %s to toggle selection\n",
        dpsoHotkeyToString(&toggleSelectionHotkey));
}


static void reportProgress(
    const DpsoOcr* ocr, DpsoOcrProgress* lastProgress)
{
    DpsoOcrProgress progress;
    int totalProgress;

    dpsoOcrGetProgress(ocr, &progress);

    if (progress.totalJobs == 0
            || dpsoOcrProgressEqual(&progress, lastProgress))
        return;

    *lastProgress = progress;

    if (progress.curJob == 0)
        totalProgress = 0;
    else
        totalProgress =
            ((progress.curJob - 1) * 100 + progress.curJobProgress)
            / progress.totalJobs;

    printf(
        "Recognition %2i%% (%i/%i)\n",
        totalProgress, progress.curJob, progress.totalJobs);
}


static void checkResults(DpsoOcr* ocr)
{
    DpsoOcrJobResults results;
    int i;

    dpsoOcrFetchResults(ocr, &results);

    for (i = 0; i < results.numItems; ++i) {
        const DpsoOcrJobResult* result = &results.items[i];
        printf(
            "=== %s ===\n%s\n",
            result->timestamp, result->text);
    }
}


static void checkHotkeyActions(DpsoOcr* ocr)
{
    const DpsoHotkeyAction hotkeyAction = dpsoGetLastHotkeyAction();
    if (hotkeyAction == hotkeyActionToggleSelection) {
        if (dpsoGetSelectionIsEnabled()) {
            DpsoOcrJobArgs jobArgs;

            dpsoSetSelectionIsEnabled(false);

            dpsoGetSelectionGeometry(&jobArgs.screenRect);
            if (dpsoRectIsEmpty(&jobArgs.screenRect))
                return;

            jobArgs.flags = dpsoOcrJobTextSegmentation;

            if (!dpsoOcrQueueJob(ocr, &jobArgs))
                fprintf(
                    stderr,
                    "dpsoQueueJob() error: %s\n", dpsoGetError());
        } else
            dpsoSetSelectionIsEnabled(true);
    }
}


volatile sig_atomic_t interrupted;


static void sigintHandler(int signum)
{
    (void)signum;
    interrupted = 1;
}


int main(void)
{
    DpsoOcrEngineInfo ocrEngineInfo;
    DpsoOcrArgs ocrArgs = {0};
    DpsoOcr* ocr;
    DpsoOcrProgress lastProgress = {0};

    if (!dpsoInit()) {
        fprintf(
            stderr, "dpsoInit() error: %s\n", dpsoGetError());
        return EXIT_FAILURE;
    }

    if (dpsoOcrGetNumEngines() == 0) {
        fprintf(stderr, "No OCR engines available\n");
        dpsoShutdown();
        exit(EXIT_FAILURE);
    }

    dpsoOcrGetEngineInfo(ocrArgs.engineIdx, &ocrEngineInfo);

    ocr = dpsoOcrCreate(&ocrArgs);
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

    signal(SIGINT, sigintHandler);
    while (!interrupted) {
        dpsoUpdate();

        reportProgress(ocr, &lastProgress);
        checkResults(ocr);
        checkHotkeyActions(ocr);

        dpsoDelay(1000 / 60);
    }

    dpsoOcrDelete(ocr);
    dpsoShutdown();

    return EXIT_SUCCESS;
}
