
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dpso/dpso.h"


static void setupLanguages(void)
{
    int langIdx;

    if (dpsoGetNumLangs() == 0) {
        fprintf(stderr, "Please install language packs\n");
        exit(EXIT_FAILURE);
    }

    langIdx = dpsoGetLangIdx(dpsoGetDefaultLangCode());
    if (langIdx == -1) {
        printf(
            "Default language (%s) is not available; using %s\n",
            dpsoGetDefaultLangCode(), dpsoGetLangCode(0));
        langIdx = 0;
    }

    dpsoSetLangIsActive(langIdx, true);
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


static void reportProgress(void)
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


static void checkResult(void)
{
    struct DpsoJobResults results;
    int i;

    if (!dpsoFetchResults(dpsoFetchFullChain))
        return;

    dpsoGetFetchedResults(&results);

    for (i = 0; i < results.numItems; ++i) {
        const struct DpsoJobResult* result = &results.items[i];
        printf(
            "=== Result %i/%i (%s) ===\n%s\n",
            i + 1, results.numItems, result->timestamp, result->text);
    }
}


static void checkHotkeyActions(void)
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


int main(void)
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
