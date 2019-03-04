
/**
 * \file
 * Configuration management
 *
 * The file provides routines to load, change, and save a
 * configuration file in a platform-specific config directory.
 *
 * Things to keep in mind:
 *
 *   * Keys must only consist of alphanumeric characters and an
 *     underscore.
 *
 *   * Values are stored as strings, and can act like variant types.
 *     For example, you can set a value as int, and then retrieve it
 *     as string. The opposite is also possible, as long as the string
 *     represents an integer (if not, a user-provided default is
 *     returned).
 */

#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Load config file.
 *
 * The function clears the current config, and then loads a file
 * cfgName for application appName. AppName will become a directory
 * of a platform-specific configuration dir. CfgName thus should be a
 * base name.
 */
void dpsoCfgLoad(const char* appName, const char* cfgName);


/**
 * Save config file.
 *
 * \sa dpsoLoadCfg()
 */
void dpsoCfgSave(const char* appName, const char* cfgName);


/**
 * Clear config.
 *
 * The function clears all key-value pairs.
 */
void dpsoCfgClear(void);


/**
 * Get string.
 *
 * The function returns the value of the key, or defaultVal if the key
 * does not exist.
 *
 * The returned pointer is valid till the next call to a function that
 * changes the config, like dpsoCfgLoad(), dpsoCfgClear(), and
 * dpsoCfgSet*().
 */
const char* dpsoCfgGetStr(const char* key, const char* defaultVal);
void dpsoCfgSetStr(const char* key, const char* val);


/**
 * Get integer.
 *
 * The function returns the value of the config field key as int. If
 * the string is a valid integer, it's value is returned. If the
 * string is "true" or "false" (ignoring case), 1 or 0 is returned
 * respectively. Otherwise, defaultVal is returned.
 */
int dpsoCfgGetInt(const char* key, int defaultVal);
void dpsoCfgSetInt(const char* key, int val);


/**
 * Get boolean.
 *
 * This function is the same as dpsoCfgGetInt(), except that it always
 * returns either 0 or 1.
 *
 * \sa dpsoCfgGetInt()
 */
int dpsoCfgGetBool(const char* key, int defaultVal);
void dpsoCfgSetBool(const char* key, int val);


#ifdef __cplusplus
}
#endif
