
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dpso/dpso.h"


static void setupLanguages(struct DpsoOcr* ocr)
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
    const struct DpsoHotkey toggleSelectionHotkey = {
        dpsoKeyGrave, dpsoKeyModCtrl
    };

    dpsoSetHotheysEnabled(true);

    dpsoBindHotkey(
        &toggleSelectionHotkey, hotkeyActionToggleSelection);
}


static void reportProgress(
    const struct DpsoOcr* ocr, struct DpsoOcrProgress* lastProgress)
{
    struct DpsoOcrProgress progress;
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


static void checkResults(struct DpsoOcr* ocr)
{
    struct DpsoOcrJobResults results;
    int i;

    dpsoOcrFetchResults(ocr, &results);

    for (i = 0; i < results.numItems; ++i) {
        const struct DpsoOcrJobResult* result = &results.items[i];
        printf(
            "=== %s ===\n%s\n",
            result->timestamp, result->text);
    }
}


static void checkHotkeyActions(struct DpsoOcr* ocr)
{
    const DpsoHotkeyAction hotkeyAction = dpsoGetLastHotkeyAction();
    if (hotkeyAction == hotkeyActionToggleSelection) {
        if (dpsoGetSelectionIsEnabled()) {
            struct DpsoOcrJobArgs jobArgs;

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


int main(void)
{
    struct DpsoOcrEngineInfo ocrEngineInfo;
    struct DpsoOcrArgs ocrArgs = {"", NULL};
    struct DpsoOcr* ocr;
    struct DpsoOcrProgress lastProgress = {0};

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

    dpsoOcrGetEngineInfo(0, &ocrEngineInfo);

    ocrArgs.engineId = ocrEngineInfo.id;
    ocrArgs.dataDir = NULL;

    ocr = dpsoOcrCreate(&ocrArgs);
    if (!ocr) {
        fprintf(
            stderr,
            "dpsoOcrCreate() with \"%s\" engine failed: %s\n",
            ocrArgs.engineId, dpsoGetError());
        dpsoShutdown();
        return EXIT_FAILURE;
    }

    setupLanguages(ocr);
    setupHotkeys();

    while (true) {
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
