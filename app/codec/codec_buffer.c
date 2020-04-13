/**
 * @file        codec_buffer.c
 * @author      Danius Kalvaitis (danius.kalvaitis@gmail.com)
 * @brief       Buffer implementation for transfering audio data to the codec.
 * @version     0.1
 * @date        2020-04-12
 * 
 * @copyright   Copyright (c) Danius Kalvaitis 2020 All rights reserved
 * 
 */

#include "codec_buffer.h"
#include "nrf_balloc.h"
#include "nrf_queue.h"

#define NRF_LOG_MODULE_NAME codec_buffer
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define CODEC_BUFFER_SIZE           CODEC_BUFFER_SIZE_WORDS * sizeof(uint32_t)

#define CODEC_POOL_SIZE             32
#define CODEC_POOL_ELEMENT_SIZE     CODEC_BUFFER_SIZE + 192
#define CODEC_QUEUE_SIZE            CODEC_POOL_SIZE
#define CODEC_QUEUE_WATERMARK_LOW   4

#define CODEC_POPPED_QUEUE_SIZE     2

typedef struct
{
	uint8_t  * p_buffer;
	size_t   size;          /**< Size of the buffer. Amount of bytes already written to it. */
	uint16_t write_index;   /**< Write index. Index of the next byte to write to the buffer. */
} codec_buffer_t;

typedef uint32_t * codec_buffer_pointer_t;

NRF_BALLOC_DEF(m_codec_pool, CODEC_POOL_ELEMENT_SIZE, CODEC_POOL_SIZE);
NRF_QUEUE_DEF(codec_buffer_pointer_t, m_codec_queue, CODEC_POOL_SIZE, NRF_QUEUE_MODE_NO_OVERFLOW);
NRF_QUEUE_DEF(codec_buffer_pointer_t, m_poped_buffer_queue, CODEC_POPPED_QUEUE_SIZE, NRF_QUEUE_MODE_NO_OVERFLOW);

static codec_buffer_t m_codec_buffer;
static codec_buffer_event_handler_t m_event_handler = NULL;
static size_t m_queue_utilization;

static void codec_buffer_alloc(codec_buffer_t * p_codec_buffer)
{
	p_codec_buffer->size        = 0;
	p_codec_buffer->write_index = 0;

	p_codec_buffer->p_buffer = nrf_balloc_alloc(&m_codec_pool);
}

ret_code_t codec_buffer_init(codec_buffer_event_handler_t event_handler)
{
	VERIFY_PARAM_NOT_NULL(event_handler);

	m_event_handler = event_handler;

	memset(&m_codec_buffer, 0, sizeof(m_codec_buffer));

	m_queue_utilization = 0;

	return nrf_balloc_init(&m_codec_pool);
}

void * codec_buffer_get_rx(size_t size)
{
	uint16_t wr_index;

	if(m_codec_buffer.p_buffer == NULL)
	{
		codec_buffer_alloc(&m_codec_buffer);

		if(m_codec_buffer.p_buffer == NULL)
		{
			NRF_LOG_WARNING("Codec buffer pool overflow");
			return NULL;
		}
	}

	if(m_codec_buffer.write_index >= CODEC_BUFFER_SIZE)
	{
		NRF_LOG_ERROR("Codec buffer pool element write index overflow %u", m_codec_buffer.write_index);
		return NULL;
	}

	wr_index = m_codec_buffer.write_index;
	m_codec_buffer.write_index += size;
	return &m_codec_buffer.p_buffer[wr_index];
}

ret_code_t codec_buffer_release_rx(size_t size)
{
	ret_code_t err_code;

	m_codec_buffer.size += size;

	if(m_codec_buffer.size >= CODEC_BUFFER_SIZE) // We filled the buffer! Time to copy its overflow to the next block and push buffer pointer to FIFO
	{
		size_t copy_size, queue_utilization;

		codec_buffer_t previous_buffer = m_codec_buffer;
		codec_buffer_alloc(&m_codec_buffer);

		if(m_codec_buffer.p_buffer == NULL)
		{
			return NRF_ERROR_NO_MEM;
		}

		copy_size = previous_buffer.size - CODEC_BUFFER_SIZE;
		memcpy(m_codec_buffer.p_buffer, &previous_buffer.p_buffer[CODEC_BUFFER_SIZE], copy_size);
		m_codec_buffer.write_index = copy_size;
		m_codec_buffer.size = copy_size;

		err_code = nrf_queue_push(&m_codec_queue, (uint32_t **)&previous_buffer.p_buffer);
		VERIFY_SUCCESS(err_code);

		queue_utilization = nrf_queue_utilization_get(&m_codec_queue);

		if(queue_utilization >= CODEC_QUEUE_WATERMARK_LOW &&
		   m_queue_utilization < CODEC_QUEUE_WATERMARK_LOW)
		{
			if(m_event_handler != NULL)
			{
				m_event_handler(CODEC_BUFFER_EVENT_TYPE_LOW_WATERMARK_CROSSED_UP);
			}
		}

		m_queue_utilization = queue_utilization;
	}

	return NRF_SUCCESS;
}

ret_code_t codec_buffer_release_rx_unfinished(void)
{
	ret_code_t err_code;
	size_t zero_data_size = CODEC_BUFFER_SIZE - m_codec_buffer.size;
	memset(&m_codec_buffer.p_buffer[m_codec_buffer.write_index], 0, zero_data_size);

	err_code = nrf_queue_push(&m_codec_queue, (uint32_t **)&m_codec_buffer.p_buffer);

	memset(&m_codec_buffer, 0, sizeof(m_codec_buffer));

	return err_code;
}

uint32_t * codec_buffer_get_tx(void)
{
	ret_code_t err_code;
	uint32_t * p_buffer;

	err_code = nrf_queue_pop(&m_codec_queue, (uint32_t **)&p_buffer);
	if(err_code != NRF_SUCCESS)
	{
		NRF_LOG_INFO("Queue empty");
		return NULL;
	}

	size_t size = nrf_queue_utilization_get(&m_poped_buffer_queue);
	if(size >= CODEC_POPPED_QUEUE_SIZE)
	{
		uint32_t * p_released_buffer;
		err_code = nrf_queue_pop(&m_poped_buffer_queue, (uint32_t **)&p_released_buffer);

		if(err_code == NRF_SUCCESS)
		{
			nrf_balloc_free(&m_codec_pool, (void *)p_released_buffer);
		}
	}

	err_code = nrf_queue_push(&m_poped_buffer_queue, (uint32_t **)&p_buffer);

	if(err_code != NRF_SUCCESS)
	{
		NRF_LOG_ERROR("Internal error");
	}

	return p_buffer;
}

void codec_buffer_reset(void)
{
	uint32_t * p_buffer;

	while(nrf_queue_pop(&m_poped_buffer_queue, (uint32_t **)&p_buffer) == NRF_SUCCESS)
	{
		nrf_balloc_free(&m_codec_pool, (void *)p_buffer);
	}

	while(nrf_queue_pop(&m_codec_queue, (uint32_t **)&p_buffer) == NRF_SUCCESS)
	{
		nrf_balloc_free(&m_codec_pool, (void *)p_buffer);
	}

	memset(&m_codec_buffer, 0, sizeof(m_codec_buffer));

	NRF_LOG_INFO("Max queue utilization %u", nrf_queue_max_utilization_get(&m_codec_queue));
	NRF_LOG_INFO("Max pool utilization %u", nrf_balloc_max_utilization_get(&m_codec_pool));

	nrf_queue_max_utilization_reset(&m_codec_queue);

	m_queue_utilization = 0;
}
