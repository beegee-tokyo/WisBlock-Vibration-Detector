#include "main.h"

/** Flag for battery protection enabled */
bool battery_check_enabled = false;

/** Set the device name, max length is 10 characters */
char g_ble_dev_name[10] = "RAK-SENS";

/** Send Fail counter **/
uint8_t join_send_fail = 0;

/** Flag for low battery protection */
bool low_batt_protection = false;

/** LoRaWAN packet */
WisCayenne g_solution_data(255);

/** State of vibration detection */
bool motion_detected = false;

/**
 * @brief Application specific setup functions
 *
 */
void setup_app(void)
{
#if API_DEBUG == 0
	// Initialize Serial for debug output
	Serial.begin(115200);

	time_t serial_timeout = millis();
	// On nRF52840 the USB serial is not available immediately
	while (!Serial)
	{
		if ((millis() - serial_timeout) < 5000)
		{
			delay(100);
			digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
		}
		else
		{
			break;
		}
	}

	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);
#endif

	// Initialize user AT commands
	init_user_at();
	
	// BLE off
	g_enable_ble = false;
}

/**
 * @brief Application specific initializations
 *
 * @return true Initialization success
 * @return false Initialization failure
 */
bool init_app(void)
{
	// Set firmware version
	api_set_version(SW_VERSION_1, SW_VERSION_2, SW_VERSION_3);
	Serial.println("========================================");
	Serial.printf("Vibration Sensor V%d.%d.%d\n", SW_VERSION_1, SW_VERSION_2, SW_VERSION_3);
	Serial.println("========================================");
	// Initialize the accelerometer
	if (init_rak1904())
	{
		Serial.println("+EVT: RAK1904 OK");
	}

	// Reset the packet
	g_solution_data.reset();

	digitalWrite(WB_IO2, LOW);

	return true;
}

/**
 * @brief Application specific event handler
 *        Requires as minimum the handling of STATUS event
 *        Here you handle as well your application specific events
 */
void app_event_handler(void)
{
	bool send_packet = false;

	// Timer triggered event
	if ((g_task_event_type & STATUS) == STATUS)
	{
		g_task_event_type &= N_STATUS;
		MYLOG("APP", "Timer wakeup");
		send_packet = true;
	}

	// Motion triggered event
	if ((g_task_event_type & MOTION) == MOTION)
	{
		g_task_event_type &= N_MOTION;
		MYLOG("APP", "Motion wakeup");
		clear_int_rak1904();

		if (!motion_detected)
		{
			motion_detected = true;
			// digitalWrite(LED_BLUE, HIGH);
			// Start the timer.
			motion_end_timeout.start();
			send_packet = true;
			MYLOG("APP", "Motion start");
			api_timer_restart(g_lorawan_settings.send_repeat_time);
		}
		else
		{
			// restart the timer
			motion_end_timeout.stop();
			motion_end_timeout.start();
			MYLOG("APP", "Motion re-trigger");
		}
	}

	if ((g_task_event_type & MOTION_END) == MOTION_END)
	{
		g_task_event_type &= N_MOTION_END;
		clear_int_rak1904();

		motion_detected = false;
		digitalWrite(LED_BLUE, LOW);
		send_packet = true;
		MYLOG("APP", "Motion end");
	}

	if (send_packet)
	{
		// Add battery voltage
		float batt_level_f = read_batt();
		g_solution_data.addVoltage(LPP_CHANNEL_BATT, batt_level_f / 1000.0);

		// Add vibration status
		g_solution_data.addPresence(LPP_CHANNEL_ACC, motion_detected);

		// Send the packet
		if (g_lorawan_settings.lorawan_enable)
		{
			if (g_lpwan_has_joined)
			{
				lmh_error_status result = send_lora_packet(g_solution_data.getBuffer(), g_solution_data.getSize());
				switch (result)
				{
				case LMH_SUCCESS:
					MYLOG("APP", "Packet enqueued");
					break;
				case LMH_BUSY:
					MYLOG("APP", "LoRa transceiver is busy");
					AT_PRINTF("+EVT:BUSY\n");
					break;
				case LMH_ERROR:
					AT_PRINTF("+EVT:SIZE_ERROR\n");
					MYLOG("APP", "Packet error, too big to send with current DR");
					break;
				}
			}
		}
		else
		{
			// Add unique identifier in front of the P2P packet, here we use the DevEUI
			g_solution_data.addDevID(0x00, &g_lorawan_settings.node_device_eui[4]);

			// Send packet over LoRa
			if (send_p2p_packet(g_solution_data.getBuffer(), g_solution_data.getSize()))
			{
				MYLOG("APP", "Packet enqueued");
			}
			else
			{
				AT_PRINTF("+EVT:SIZE_ERROR\n");
				MYLOG("APP", "Packet too big");
			}
		}
	}
	// else
	// {
	// 	MYLOG("APP", "Wake up reason %X", g_task_event_type);
	// }
	// Reset the packet
	g_solution_data.reset();
}

/**
 * @brief Handle BLE UART data
 *
 */
void ble_data_handler(void)
{
	if (g_enable_ble)
	{
		// BLE UART data handling
		if ((g_task_event_type & BLE_DATA) == BLE_DATA)
		{
			MYLOG("AT", "RECEIVED BLE");
			/** BLE UART data arrived */
			g_task_event_type &= N_BLE_DATA;

			while (g_ble_uart.available() > 0)
			{
				at_serial_input(uint8_t(g_ble_uart.read()));
				delay(5);
			}
			at_serial_input(uint8_t('\n'));
		}
	}
}

/**
 * @brief Handle received LoRa Data
 *
 */
void lora_data_handler(void)
{
	// LoRa Join finished handling
	if ((g_task_event_type & LORA_JOIN_FIN) == LORA_JOIN_FIN)
	{
		g_task_event_type &= N_LORA_JOIN_FIN;
		if (g_join_result)
		{
			MYLOG("APP", "Successfully joined network");
			AT_PRINTF("+EVT:JOINED\n");
		}
		else
		{
			MYLOG("APP", "Join network failed");
			AT_PRINTF("+EVT:JOIN FAILED\n");
			/// \todo here join could be restarted.
			lmh_join();
		}
	}

	// LoRa TX finished handling
	if ((g_task_event_type & LORA_TX_FIN) == LORA_TX_FIN)
	{
		g_task_event_type &= N_LORA_TX_FIN;

		MYLOG("APP", "LoRa TX cycle %s", g_rx_fin_result ? "finished ACK" : "failed NAK");

		if ((g_lorawan_settings.confirmed_msg_enabled) && (g_lorawan_settings.lorawan_enable))
		{
			AT_PRINTF("+EVT:SEND CONFIRMED %s\n", g_rx_fin_result ? "OK" : "ERROR");
		}
		else
		{
			AT_PRINTF("+EVT:SEND OK\n");
		}

		if (!g_rx_fin_result)
		{
			// Increase fail send counter
			join_send_fail++;

			if (join_send_fail == 10)
			{
				// Too many failed sendings, reset node and try to rejoin
				delay(100);
				api_reset();
			}
		}
	}

	// LoRa data handling
	if ((g_task_event_type & LORA_DATA) == LORA_DATA)
	{
		g_task_event_type &= N_LORA_DATA;
		MYLOG("APP", "Received package over LoRa");
		// Check if uplink was a send frequency change command
		if ((g_last_fport == 3) && (g_rx_data_len == 6))
		{
			if (g_rx_lora_data[0] == 0xAA)
			{
				if (g_rx_lora_data[1] == 0x55)
				{
					uint32_t new_send_frequency = 0;
					new_send_frequency |= (uint32_t)(g_rx_lora_data[2]) << 24;
					new_send_frequency |= (uint32_t)(g_rx_lora_data[3]) << 16;
					new_send_frequency |= (uint32_t)(g_rx_lora_data[4]) << 8;
					new_send_frequency |= (uint32_t)(g_rx_lora_data[5]);

					MYLOG("APP", "Received new send frequency %ld s\n", new_send_frequency);
					// Save the new send frequency
					g_lorawan_settings.send_repeat_time = new_send_frequency * 1000;

					// Set the timer to the new send frequency
					api_timer_restart(g_lorawan_settings.send_repeat_time);
					// Save the new send frequency
					save_settings();
				}
			}
		}

		if (g_lorawan_settings.lorawan_enable)
		{
			AT_PRINTF("+EVT:RX_1, RSSI %d, SNR %d\n", g_last_rssi, g_last_snr);
			AT_PRINTF("+EVT:%d:", g_last_fport);
			for (int idx = 0; idx < g_rx_data_len; idx++)
			{
				AT_PRINTF("%02X", g_rx_lora_data[idx]);
			}
			AT_PRINTF("\n");
		}
		else
		{
			AT_PRINTF("+EVT:RXP2P, RSSI %d, SNR %d\n", g_last_rssi, g_last_snr);
			AT_PRINTF("+EVT:");
			for (int idx = 0; idx < g_rx_data_len; idx++)
			{
				AT_PRINTF("%02X", g_rx_lora_data[idx]);
			}
			AT_PRINTF("\n");
		}
	}
}
