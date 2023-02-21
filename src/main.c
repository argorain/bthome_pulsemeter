/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>

#define SERVICE_DATA_LEN        9
#define SERVICE_UUID            0xfcd2		// BTHome service UUID

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)


#define ADV_PARAM BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, \
					     BT_GAP_ADV_SLOW_INT_MIN, \
					     BT_GAP_ADV_SLOW_INT_MAX, NULL)

static uint8_t service_data[SERVICE_DATA_LEN] = { 
	BT_UUID_16_ENCODE(SERVICE_UUID),
	0x40,
	0x02,
	0xc4,
	0x09,
	0x03,
	0xbf,
	0x13,
};

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
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

void main(void)
{
	int err;

	printk("Starting BTHome test\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	printk("Bluetooth initialized\n");

	for (;;) {
		// Change adv data here:
		// eg. temperature
		err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			printk("Failed to update advertising data (err %d)\n", err);
		}
		k_sleep(K_MSEC(BT_GAP_ADV_SLOW_INT_MIN));
	}
}
