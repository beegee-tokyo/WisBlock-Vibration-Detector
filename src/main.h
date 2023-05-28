/**
 * @file main.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Includes, defines and globals
 * @version 0.1
 * @date 2023-05-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <Arduino.h>
#include <Wisblock-API-V2.h>

// Debug
// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 1
#endif

#if MY_DEBUG > 0
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\n");             \
	} while (0);                         \
	delay(100)
#else
#define MYLOG(...)
#endif

/** Wakeup triggers for application events */
#define MOTION       0b1000000000000000
#define N_MOTION     0b0111111111111111
#define MOTION_END   0b0100000000000000
#define N_MOTION_END 0b1011111111111111

// Cayenne LPP channel numbers
#define LPP_CHANNEL_BATT 1 // Base Board
#define LPP_CHANNEL_ACC 48 // RAK1904 ACC interrupt

// Forward declarations
bool init_rak1904(void);
void clear_int_rak1904(void);
extern SoftwareTimer motion_end_timeout;

// User AT commands
void init_user_at(void);
void read_settings(void);
void save_settings(uint32_t tout);
extern uint32_t g_tout;