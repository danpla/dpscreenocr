
#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * DpsoCfg.
 *
 * The format of the configuration file is designed to be simple and
 * human-friendly. Each line contains a key-value pair; empty lines
 * are ignored. The first word is a key, and the rest is a value.
 * Spaces and tabs around both the key and the value are ignored. The
 * value can have \n, \r, and \t escape sequences. Other characters
 * preceded by \ are kept as is; \ at the end of the line is ignored.
 * To preserve leading spaces, escape the first one; to preserve
 * trailing spaces, either escape the last one or put \ after it at
 * the end of the line (dpsoCfgSave() will do the latter).
 *
 * Regarding the API, the file format implies that the key should not
 * be empty or contain a space, \t, \r, or \n. The functions will do
 * nothing on attempt to set such a key.
 */
typedef struct DpsoCfg DpsoCfg;


/**
 * Create an empty Cfg.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * null.
 */
DpsoCfg* dpsoCfgCreate(void);


void dpsoCfgDelete(DpsoCfg* cfg);


/**
 * Load config file.
 *
 * The function clears the config and loads the filePath file.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false. Nonexistent filePath is not considered an error.
 */
bool dpsoCfgLoad(DpsoCfg* cfg, const char* filePath);


/**
 * Save config file.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 *
 * \sa dpsoLoadCfg()
 */
bool dpsoCfgSave(const DpsoCfg* cfg, const char* filePath);


/**
 * Clear config.
 *
 * The function clears all key-value pairs.
 */
void dpsoCfgClear(DpsoCfg* cfg);


/**
 * Check if the given key exists.
 */
bool dpsoCfgKeyExists(const DpsoCfg* cfg, const char* key);


/**
 * Get string.
 *
 * The function returns the value of the key, or defaultVal if the key
 * does not exist. You can use null for defaultVal to distinguish
 * between an empty value and a nonexistent key (or explicitly call
 * dpsoCfgKeyExists() instead).
 *
 * The returned pointer is valid till the next call to a function that
 * changes the config, like dpsoCfgLoad(), dpsoCfgClear(), and
 * dpsoCfgSet*().
 */
const char* dpsoCfgGetStr(
    const DpsoCfg* cfg, const char* key, const char* defaultVal);
void dpsoCfgSetStr(DpsoCfg* cfg, const char* key, const char* val);


/**
 * Get integer.
 *
 * The function returns the value of the key parsed as int. If the
 * value does not represent a decimal number, defaultVal is returned.
 */
int dpsoCfgGetInt(
    const DpsoCfg* cfg, const char* key, int defaultVal);
void dpsoCfgSetInt(DpsoCfg* cfg, const char* key, int val);


/**
 * Get boolean.
 *
 * The function returns bool depending on whether the value of the key
 * is "true" or "false", ignoring case. For other values, defaultVal
 * is returned.
 */
bool dpsoCfgGetBool(
    const DpsoCfg* cfg, const char* key, bool defaultVal);
void dpsoCfgSetBool(DpsoCfg* cfg, const char* key, bool val);


#ifdef __cplusplus
}


#include <memory>


namespace dpso {


struct CfgDeleter {
    void operator()(DpsoCfg* cfg) const
    {
        dpsoCfgDelete(cfg);
    }
};


using CfgUPtr = std::unique_ptr<DpsoCfg, CfgDeleter>;


}


#endif
