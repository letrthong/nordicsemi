/*
 *  Copyright (c) 2021, PACKETCRAFT, INC.
 *
 *  SPDX-License-Identifier: LicenseRef-PCFT
 */

#include "audio_i2s.h"

#include "sd_card.h"


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <nrfx_i2s.h>
#include <nrfx_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2s, CONFIG_MAIN_LOG_LEVEL);


#include "audio_sync_timer.h"

#define I2S_NL DT_NODELABEL(i2s0)

#define HFCLKAUDIO_12_288_MHZ 0x9BAE

enum audio_i2s_state {
	AUDIO_I2S_STATE_UNINIT,
	AUDIO_I2S_STATE_IDLE,
	AUDIO_I2S_STATE_STARTED,
};

static enum audio_i2s_state state = AUDIO_I2S_STATE_UNINIT;

PINCTRL_DT_DEFINE(I2S_NL);

#if CONFIG_AUDIO_SAMPLE_RATE_16000_HZ
#define CONFIG_AUDIO_RATIO NRF_I2S_RATIO_384X
#elif CONFIG_AUDIO_SAMPLE_RATE_24000_HZ
#define CONFIG_AUDIO_RATIO NRF_I2S_RATIO_256X
#elif CONFIG_AUDIO_SAMPLE_RATE_48000_HZ
#define CONFIG_AUDIO_RATIO NRF_I2S_RATIO_128X
#else
#error "Current AUDIO_SAMPLE_RATE_HZ setting not supported"
#endif

static nrfx_i2s_config_t cfg = {
	/* Pins are configured by pinctrl. */
	.skip_gpio_cfg = true,
	.skip_psel_cfg = true,
	.irq_priority = DT_IRQ(I2S_NL, priority),
	.mode = NRF_I2S_MODE_MASTER,
	.format = NRF_I2S_FORMAT_I2S,
	.alignment = NRF_I2S_ALIGN_LEFT,
	//.ratio = CONFIG_AUDIO_RATIO,
	.ratio = 128,
	.mck_setup = 0x66666000,
//#if (CONFIG_AUDIO_BIT_DEPTH_16)
//	.sample_width = NRF_I2S_SWIDTH_16BIT,
//#elif (CONFIG_AUDIO_BIT_DEPTH_32)
//	.sample_width = NRF_I2S_SWIDTH_32BIT,
//#else
//	.sample_width = NRF_I2S_SWIDTH_32BIT,
//#error Invalid bit depth selected
//#endif /* (CONFIG_AUDIO_BIT_DEPTH_16) */
	.sample_width = NRF_I2S_SWIDTH_32BIT,
	.channels = NRF_I2S_CHANNELS_STEREO,
	.clksrc = NRF_I2S_CLKSRC_ACLK,
	.enable_bypass = false,
};

//ThongLT
static nrfx_i2s_buffers_t initial_buffers;
#define I2S_DATA_BLOCK_WORDS 128
static bool data_ready_flag = false;
static uint32_t  m_buffer_rx32u[I2S_DATA_BLOCK_WORDS];
static  uint32_t  tmp[I2S_DATA_BLOCK_WORDS];
static i2s_blk_comp_callback_t i2s_blk_comp_callback;

static void i2s_comp_handler(nrfx_i2s_buffers_t const *released_bufs, uint32_t status)
{
	//LOG_INF("i2s_comp_handler callback");
	// if ((status == NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED) && released_bufs &&
	//     i2s_blk_comp_callback && (released_bufs->p_rx_buffer || released_bufs->p_tx_buffer)) {
	// 	i2s_blk_comp_callback(audio_sync_timer_i2s_frame_start_ts_get(),
	// 							released_bufs->p_rx_buffer, 
	// 							released_bufs->p_tx_buffer);
	// }

	if (NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED == status)
	{
		nrfx_err_t err = nrfx_i2s_next_buffers_set(&initial_buffers);
		if (err != NRFX_SUCCESS)
		{
			//printk("Error!, continuing running as if nothing happened, but you should probably investigate.\n");
		}
	}

	//https://github.com/siguhe/NCS_I2S_nrfx_driver_example/blob/master/src/main.c
	if (released_bufs)
	{
		if (released_bufs->p_rx_buffer != NULL)
		{
			static int connt = 0;
			connt = connt +1;
			if(connt >=100){
				 
				//LOG_INF("i2s_comp_handler p_rx_buffer start");
			} 
			
			//data_ready_flag = true; //This is used in print_sound()
			//size_t  size = I2S_SAMPLES_NUM;
			//sd_card_write("test.wav", released_bufs->p_rx_buffer, &size);

			for (int i = 0; i < I2S_DATA_BLOCK_WORDS; i++)
			{
				uint32_t data = released_bufs->p_rx_buffer+i;
		        // LOG_INF("i2s_comp_handler data=[%d] i=%d \n",  data,  i); 
				//memcpy(tmp[i] , released_bufs->p_rx_buffer , sizeof(uint32_t)*I2S_DATA_BLOCK_WORDS );
				//memcpy(tmp+i ,&data, sizeof(uint32_t)  );
				tmp[i] = data;
				//LOG_INF("i2s_comp_handler tmp[i]=[%d] i=%d \n",  tmp[i],  i); 
				 
			}

			data_ready_flag = true;
			
			 if(connt >=100){
				connt = 0;
				//LOG_INF("i2s_comp_handler p_rx_buffer end");
			} 
		}
	}
}

void audio_i2s_set_next_buf(const uint8_t *tx_buf, uint32_t *rx_buf)
{
	//LOG_INF("audio_i2s_set_next_buf");
	__ASSERT_NO_MSG(state == AUDIO_I2S_STATE_STARTED);
	__ASSERT_NO_MSG(rx_buf != NULL);
#if (CONFIG_STREAM_BIDIRECTIONAL || (CONFIG_AUDIO_DEV == HEADSET))
	__ASSERT_NO_MSG(tx_buf != NULL);
#endif /* (CONFIG_STREAM_BIDIRECTIONAL || (CONFIG_AUDIO_DEV == HEADSET)) */

	const nrfx_i2s_buffers_t i2s_buf = { .p_rx_buffer = rx_buf,
					     .p_tx_buffer = (uint32_t *)tx_buf };

	nrfx_err_t ret;

	ret = nrfx_i2s_next_buffers_set(&i2s_buf);
	__ASSERT_NO_MSG(ret == NRFX_SUCCESS);
}

void audio_i2s_start(const uint8_t *tx_buf, uint32_t *rx_buf)
{
	LOG_INF("audio_i2s_start start");
	__ASSERT_NO_MSG(state == AUDIO_I2S_STATE_IDLE);
	__ASSERT_NO_MSG(rx_buf != NULL);
#if (CONFIG_STREAM_BIDIRECTIONAL || (CONFIG_AUDIO_DEV == HEADSET))
	__ASSERT_NO_MSG(tx_buf != NULL);
#endif /* (CONFIG_STREAM_BIDIRECTIONAL || (CONFIG_AUDIO_DEV == HEADSET)) */

	const nrfx_i2s_buffers_t i2s_buf = { .p_rx_buffer = rx_buf,
					     .p_tx_buffer = (uint32_t *)tx_buf };

	nrfx_err_t ret;

	/* Buffer size in 32-bit words */
	ret = nrfx_i2s_start(&i2s_buf, I2S_SAMPLES_NUM, 0);
	__ASSERT_NO_MSG(ret == NRFX_SUCCESS);

	state = AUDIO_I2S_STATE_STARTED;

	LOG_INF("audio_i2s_start end");
}

void audio_i2s_stop(void)
{
	__ASSERT_NO_MSG(state == AUDIO_I2S_STATE_STARTED);

	nrfx_i2s_stop();

	state = AUDIO_I2S_STATE_IDLE;
}

void audio_i2s_blk_comp_cb_register(i2s_blk_comp_callback_t blk_comp_callback)
{
	i2s_blk_comp_callback = blk_comp_callback;
}

void audio_i2s_init(void)
{
	LOG_INF("audio_i2s_init start ");
	__ASSERT_NO_MSG(state == AUDIO_I2S_STATE_UNINIT);

	nrfx_err_t ret;

	nrfx_clock_hfclkaudio_config_set(HFCLKAUDIO_12_288_MHZ);

	NRF_CLOCK->TASKS_HFCLKAUDIOSTART = 1;

	/* Wait for ACLK to start */
	while (!NRF_CLOCK_EVENT_HFCLKAUDIOSTARTED) {
		k_sleep(K_MSEC(1));
	}
   
   LOG_INF("pinctrl_apply_state");
	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(I2S_NL), PINCTRL_STATE_DEFAULT);
	__ASSERT_NO_MSG(ret == 0);

	IRQ_CONNECT(DT_IRQN(I2S_NL), DT_IRQ(I2S_NL, priority), nrfx_isr, nrfx_i2s_irq_handler, 0);
	irq_enable(DT_IRQN(I2S_NL));

	LOG_INF("nrfx_i2s_init");
	

	ret = nrfx_i2s_init(&cfg, i2s_comp_handler);
	__ASSERT_NO_MSG(ret == NRFX_SUCCESS);

	state = AUDIO_I2S_STATE_IDLE;
	LOG_INF("audio_i2s_init end");
 
}


void audio_system_record_start(){
	memset(&m_buffer_rx32u, 0x00, sizeof(m_buffer_rx32u));
	initial_buffers.p_rx_buffer = m_buffer_rx32u;
	

	LOG_INF("audio_system_record_start \n");
	nrfx_err_t  err_code = nrfx_i2s_start(&initial_buffers, I2S_DATA_BLOCK_WORDS, 0); //start recording
	if (err_code != NRFX_SUCCESS)
	{
		printk("I2S start error\n");
		//return err_code;
	}
}

static int count = 0;
void audio_system_record_raw(){
	// LOG_INF("audio_system_record_raw start\n"); 
	while (!data_ready_flag)
	{
		k_sleep(K_MSEC(1));
		//Wait for data. Since we do not want I2S_DATA_BLOCK_WORDS amount of prints inside the interrupt.
	}
	//  nrfx_i2s_stop();
	data_ready_flag = false;
	
	//size_t  size = I2S_DATA_BLOCK_WORDS;
	static int connt1 = 0;
	connt1 = connt1 +1;
	if(connt1 >=100){
		connt1 = 0;
		LOG_INF("audio_system_record_raw count=%d\n", count); 
	} 
	
	char buf[I2S_DATA_BLOCK_WORDS*4];
	size_t size = I2S_DATA_BLOCK_WORDS * 4;
	for (int i = 0; i < I2S_DATA_BLOCK_WORDS; i++)
	{
		uint32_t data = tmp[i];
		//size_t  size = sizeof(uint32_t);
		
		
		buf[i*2] =  data   & 0XFF;
		buf[(i*2)+1] = (data >> 8) & 0XFF;
		buf[(i*2)+2] = (data >> 16) & 0XFF;
		buf[(i*2)+3] = (data >> 24) & 0XFF;
		
		 //LOG_INF("audio_system_record_raw %x-%x-%x-%x index=%d\n", buf[0], buf[1], buf[2],buf[3], i); 
		//  k_sleep(K_MSEC(100));
		 
		 
	}

	if (count < 10000) {
		LOG_INF("audio_system_record_raw store sdcard\n"); 
		sd_card_write("test.raw", buf, &size);
	}
	

    //audio_system_record_start();
	
	//k_sleep(K_MSEC(10));
	count = count + 1;
	if (count == 10000) {
		sd_card_close();
		LOG_INF("audio_system_record_raw  close file\n"); 
	}
}