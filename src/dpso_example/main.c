
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dpso/dpso.h"


static void setupLanguages()
{
    int engLangIdx;

    if (dpsoGetNumLangs() == 0) {
        fprintf(
            stderr, "Please install languages for Tesseract\n");
        exit(EXIT_FAILURE);
    }

    engLangIdx = dpsoGetLangIdx("eng", -1);
    if (engLangIdx != -1) {
        dpsoSetLangIsActive(engLangIdx, true);
        return;
    }

    printf(
        "English is not available; using %s\n", dpsoGetLangCode(0));
    dpsoSetLangIsActive(0, true);
}


enum HotkeyActions {
    hotkeyActionToggleSelection
};


static void setupHotkeys()
{
    const struct DpsoHotkey toggleSelectionHotkey = {
        dpsoKeyGrave, dpsoKeyModCtrl
    };

    dpsoSetHotheysEnabled(true);

    dpsoBindHotkey(
        &toggleSelectionHotkey, hotkeyActionToggleSelection);
}


static void reportProgress()
{
    struct DpsoProgress progress;
    int progressIsNew;
    int totalProgress;

    dpsoGetProgress(&progress, &progressIsNew);

    if (!progressIsNew || progress.totalJobs == 0)
        return;

    if (progress.curJob == 0)
        totalProgress = 0;
    else
        totalProgress = (
            ((progress.curJob - 1) * 100 + progress.curJobProgress)
            / progress.totalJobs);

    printf(
        "Recognition %2i%% (%i/%i)\n",
        totalProgress, progress.curJob, progress.totalJobs);
}


static void checkResult()
{
    const struct DpsoJobResult* results;
    int numResults;
    int i;

    if (!dpsoFetchResults(dpsoFetchFullChain))
        return;

    dpsoGetFetchedResults(&results, &numResults);

    for (i = 0; i < numResults; ++i) {
        const struct DpsoJobResult* result = &results[i];
        printf(
            "=== Result %i/%i (%s) ===\n%s\n",
            i + 1, numResults, result->timestamp, result->text);
    }
}


static void checkHotkeyActions()
{
    const DpsoHotkeyAction hotkeyAction = dpsoGetLastHotkeyAction();
    if (hotkeyAction == hotkeyActionToggleSelection) {
        if (dpsoGetSelectionIsEnabled()) {
            struct DpsoJobArgs jobArgs;

            dpsoSetSelectionIsEnabled(false);

            dpsoGetSelectionGeometry(&jobArgs.screenRect);
            jobArgs.flags = dpsoJobTextSegmentation;

            dpsoQueueJob(&jobArgs);
        } else
            dpsoSetSelectionIsEnabled(true);
    }
}


int main()
{
    if (!dpsoInit()) {
        fprintf(
            stderr, "dpsoInit() error: %s\n", dpsoGetError());
        exit(EXIT_FAILURE);
    }

    setupLanguages();
    setupHotkeys();

    while (true) {
        dpsoUpdate();

        reportProgress();
        checkResult();
        checkHotkeyActions();

        dpsoDelay(1000 / 60);
    }

    dpsoShutdown();

    return EXIT_SUCCESS;
}
