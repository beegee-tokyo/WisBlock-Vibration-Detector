/**
 * @file user_at_cmd.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Handle user defined AT commands
 * @version 0.4
 * @date 2022-01-29
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "main.h"
#ifdef NRF52_SERIES
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
using namespace Adafruit_LittleFS_Namespace;

/** Filename to save battery check setting */
static const char tout_name[] = "TOUT";

/** File to save battery check status */
File tout_check(InternalFS);

#endif
#ifdef ESP32
#include <Preferences.h>
/** ESP32 preferences */
Preferences esp32_prefs;
#endif

/*****************************************
 * Timeout settings
 *****************************************/

/**
 * @brief Set vibration timeout
 *
 * @param str selected timeout as String in milliseconds
 * @return int AT_SUCCESS if ok, AT_ERRNO_PARA_FAIL if invalid value
 */
static int at_set_tout(char *str)
{
	long new_tout = strtol(str, NULL, 0);

	if (new_tout > g_lorawan_settings.send_repeat_time)
	{
		return AT_ERRNO_PARA_NUM;
	}
	g_tout = new_tout;
	save_settings(g_tout);
	motion_end_timeout.setPeriod(g_tout);
	return AT_SUCCESS;
}

/**
 * @brief Get bibration timeouot
 *
 * @return int AT_SUCCESS
 */
int at_query_tout(void)
{
	AT_PRINTF("%ld", g_tout);
	return AT_SUCCESS;
}

/**
 * @brief List of all available commands with short help and pointer to functions
 *
 */
atcmd_t g_user_at_cmd_list_tout[] = {
	/*|    CMD    |     AT+CMD?      |    AT+CMD=?    |  AT+CMD=value |  AT+CMD  | Permissions |*/
	// Module commands
	{"+TOUT", "Set/get vibration timeout (ms)", at_query_tout, at_set_tout, NULL, "RW"},
};

/**
 * @brief Read saved settings
 *
 */
void read_settings(void)
{
#ifdef NRF52_SERIES
	if (InternalFS.exists(tout_name))
	{
		tout_check.open(tout_name, FILE_O_READ);
		if (!tout_check)
		{
			MYLOG("USR_AT", "Error reading file, set timeout to 5 seconds");
			g_tout = 5000;
			save_settings(5000);
		}
		else
		{
			tout_check.read((void *)&g_tout, 4);
			tout_check.close();
		}
		MYLOG("USR_AT", "Vibration timeout set to %ld", g_tout);
	}
	else
	{
		MYLOG("USR_AT", "File not found, set timeout to 5 seconds");
		g_tout = 5000;
		save_settings(5000);
	}
#endif
#ifdef ESP32
	esp32_prefs.begin("tout", false);
	g_tout = esp32_prefs.getLong("tout", 5000);
	esp32_prefs.end();
#endif
}

/**
 * @brief Save the UI settings
 *
 */
void save_settings(uint32_t tout)
{
#ifdef NRF52_SERIES
	tout_check.open(tout_name, FILE_O_WRITE);
	tout_check.write((uint8_t const *)&g_tout, 4);
	tout_check.close();
	MYLOG("USR_AT", "Saved vibration timeout");
#endif
#ifdef ESP32
	esp32_prefs.begin("tout", false);
	esp32_prefs.putLong("tout", g_tout);
	esp32_prefs.end();
#endif
}

/** Number of user defined AT commands */
uint8_t g_user_at_cmd_num = 0;

/** Pointer to the combined user AT command structure */
atcmd_t *g_user_at_cmd_list;

/**
 * @brief Initialize the user defined AT command list
 *
 */
void init_user_at(void)
{
	uint16_t index_next_cmds = 0;
	uint16_t required_structure_size = sizeof(g_user_at_cmd_list_tout);
	MYLOG("USR_AT", "Structure size %d Vibration Timeout", required_structure_size);

	// Reserve memory for the structure
	g_user_at_cmd_list = (atcmd_t *)malloc(required_structure_size);

	// Add AT commands to structure
	MYLOG("USR_AT", "Adding timeout AT commands");
	g_user_at_cmd_num += sizeof(g_user_at_cmd_list_tout) / sizeof(atcmd_t);
	memcpy((void *)&g_user_at_cmd_list[index_next_cmds], (void *)g_user_at_cmd_list_tout, sizeof(g_user_at_cmd_list_tout));
	index_next_cmds += sizeof(g_user_at_cmd_list_tout) / sizeof(atcmd_t);
}
