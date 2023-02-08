/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/gpio.h>
#include <init.h>
#include <nrf.h>
#include <nrfx.h>
#include <nrfx_gpiote.h>
#include <drivers/i2s.h>
#include "nrfx_i2s.h"
#include <errno.h>
#include <string.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(Main);

#define AUDIO_SAMPLE_FREQ (44100)
#define AUDIO_NUM_CHANNELS (1)
#define AUDIO_SAMPLE_BIT_WIDTH (16)
#define AUDIO_SAMPLES_PER_CH_PER_FRAME (128)
#define AUDIO_SAMPLES_PER_FRAME (AUDIO_SAMPLES_PER_CH_PER_FRAME * AUDIO_NUM_CHANNELS)
#define AUDIO_SAMPLE_BYTES (sizeof(uint16_t))
#define AUDIO_FRAME_BUF_BYTES (AUDIO_SAMPLES_PER_FRAME * AUDIO_SAMPLE_BYTES)
#define I2S_PLAY_BUF_COUNT (500)

static const struct device *host_i2s_tx_dev;
static struct k_mem_slab i2s_tx_mem_slab;
static char tx_buffer[AUDIO_FRAME_BUF_BYTES * I2S_PLAY_BUF_COUNT];
static struct i2s_config config;
volatile int ret;

static void init(void)
{
	host_i2s_tx_dev = device_get_binding("I2S_0");

	k_mem_slab_init(&i2s_tx_mem_slab, tx_buffer, AUDIO_FRAME_BUF_BYTES, I2S_PLAY_BUF_COUNT);
	if (ret < 0)
	{
		LOG_INF("mem slab init: %d", ret);
	}

	config.word_size = AUDIO_SAMPLE_BIT_WIDTH;
	config.channels = AUDIO_NUM_CHANNELS;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	config.options = (I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER);
	config.frame_clk_freq = AUDIO_SAMPLE_FREQ;
	config.block_size = AUDIO_FRAME_BUF_BYTES;
	config.mem_slab = &i2s_tx_mem_slab;
	config.timeout = -1;

	ret = i2s_configure(host_i2s_tx_dev, I2S_DIR_TX, &config);
	if (ret < 0)
	{
		LOG_INF("tx config: %d", ret);
	}
}

void main(void)

{
	init();

	uint16_t buf[128];
	size_t size = sizeof(buf);

	int i;
	for (i = 0; i < size; i++)
	{
		if (i <= size / 4)
		{
			buf[i] = i;
		}
		else if (i > size / 4)
		{
			buf[i] = size / 2 - i;
		}
	}

	void *tx_mem_block;

	ret = i2s_trigger(host_i2s_tx_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret < 0)
	{
		LOG_INF("trigger: %d", ret);
	}


	while (1)
	{
		ret = k_mem_slab_alloc(&i2s_tx_mem_slab, &tx_mem_block, K_NO_WAIT);
		if (ret < 0)
		{
			LOG_INF("mem slab alloc: %d", ret);
		}

		memcpy(tx_mem_block, buf, size);

		ret = i2s_write(host_i2s_tx_dev, tx_mem_block, size);
		if (ret < 0)
		{
			LOG_INF("write: %d", ret);
		}
		
		k_mem_slab_free(&i2s_tx_mem_slab, &tx_mem_block);
	}
}