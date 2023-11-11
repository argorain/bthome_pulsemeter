/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>
#include <soc.h>
#include "retained.h"
#include <hal/nrf_gpio.h>

#define SERVICE_UUID            0xfcd2      // BTHome service UUID



#define ADV_PARAM BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, \
					     BT_GAP_ADV_SLOW_INT_MIN, \
					     BT_GAP_ADV_SLOW_INT_MAX, NULL)

static uint8_t service_data[] = { 
	BT_UUID_16_ENCODE(SERVICE_UUID),
	0x44,	// bit 0 = 0 -> no encryption, bit 2 = 1 -> irregular data updates, bits 5-7 = 0b010 -> BTHome v2
	0x3E,	// Count, 4B
	0x00,	// Lowest byte
	0x00,   // 
	0x00,	// 
	0x00,	// Highest byte
	0x13,
};

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
	BT_DATA(BT_DATA_SVC_DATA16, service_data, ARRAY_SIZE(service_data))
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(ADV_PARAM, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
}



#define SLEEP_S 1U

int main(void)
{
	int rc;
	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(cons)) {
		printk("%s: device not ready.\n", cons->name);
		return 0;
	}

	bool retained_ok = retained_validate();

	/* Increment for this boot attempt and update. */
	retained.boots += 1;
	retained_update();

	printk("Retained data: %s\n", retained_ok ? "valid" : "INVALID");
	printk("Boot count: %u\n", retained.boots);
	printk("Off count: %u\n", retained.off_count);
	printk("Active Ticks: %" PRIu64 "\n", retained.uptime_sum);

	/* Initialize the Bluetooth Subsystem */
	rc = bt_enable(bt_ready);
	if (rc) {
		printk("Bluetooth init failed (err %d)\n", rc);
		return -1;
	}

	service_data[7] = (retained.boots  >> 24) & 0xff;
	service_data[6] = (retained.boots  >> 16) & 0xff;
	service_data[5] = (retained.boots  >> 8) & 0xff;
	service_data[4] = (retained.boots  >> 0) & 0xff;

	rc = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
	if (rc) {
		printk("Failed to update advertising data (err %d)\n", rc);
	}

	/* Configure to generate PORT event (wakeup) on button 1 press. */
	nrf_gpio_cfg_input(NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(sw0), gpios),
			   NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_sense_set(NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(sw0), gpios),
			       NRF_GPIO_PIN_SENSE_LOW);

	printk("Sleep %u s\n", SLEEP_S);
	k_sleep(K_SECONDS(SLEEP_S));

	printk("Entering system off; press BUTTON1 to restart\n");

	/* Update the retained state */
	retained.off_count += 1;
	retained_update();

	sys_poweroff();

	return 0;
}