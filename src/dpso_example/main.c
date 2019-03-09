
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dpso/dpso.h"


static void setupLanguages()
{
    int i;

    if (dpsoGetNumLangs() == 0) {
        fprintf(
            stderr, "Please install languages for Tesseract\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < dpsoGetNumLangs(); ++i)
        if (strcmp(dpsoGetLangCode(i), "eng") == 0) {
            dpsoSetLangIsActive(i, true);
            break;
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

    if (!dpsoFetchResults(true))
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
            int jobFlags = dpsoJobTextSegmentation;
            int x, y, w, h;

            dpsoGetSelectionGeometry(&x, &y, &w, &h);
            dpsoSetSelectionIsEnabled(false);

            dpsoQueueJob(x, y, w, h, jobFlags);
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
