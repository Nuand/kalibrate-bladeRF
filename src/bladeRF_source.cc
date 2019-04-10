/*
 * Copyright (c) 2010, Joshua Lackey
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     *  Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *     *  Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <complex>



#include "bladeRF_source.h"

extern int g_verbosity;


bladeRF_source::bladeRF_source(float sample_rate, long int fpga_master_clock_freq) {

	m_fpga_master_clock_freq = fpga_master_clock_freq;
	m_desired_sample_rate = sample_rate;
	m_sample_rate = 0.0;
	m_decimation = 0;
	m_cb = new circular_buffer(CB_LEN, sizeof(complex), 0);

	pthread_mutex_init(&m_u_mutex, 0);
}


bladeRF_source::bladeRF_source(unsigned int decimation, long int fpga_master_clock_freq) {

	m_fpga_master_clock_freq = fpga_master_clock_freq;
	m_sample_rate = 0.0;
	m_cb = new circular_buffer(CB_LEN, sizeof(complex), 0);

	pthread_mutex_init(&m_u_mutex, 0);

	m_decimation = decimation & ~1;
	if(m_decimation < 4)
		m_decimation = 4;
	if(m_decimation > 256)
		m_decimation = 256;
}


bladeRF_source::~bladeRF_source() {

	stop();
	delete m_cb;
	pthread_mutex_destroy(&m_u_mutex);
}


void bladeRF_source::stop() {

	pthread_mutex_lock(&m_u_mutex);
#if 0
	if(m_db_rx)
		m_db_rx->set_enable(0);
	if(m_u_rx)
		m_u_rx->stop();
#endif
	pthread_mutex_unlock(&m_u_mutex);
}


void bladeRF_source::start() {

	pthread_mutex_lock(&m_u_mutex);
	if (bladerf_enable_module(bdev, BLADERF_MODULE_RX, 1)) {
	}

	pthread_mutex_unlock(&m_u_mutex);
}


void bladeRF_source::calculate_decimation() {

	float decimation_f;

#if 0
	decimation_f = (float)m_u_rx->fpga_master_clock_freq() / m_desired_sample_rate;
	m_decimation = (unsigned int)round(decimation_f) & ~1;

	if(m_decimation < 4)
		m_decimation = 4;
	if(m_decimation > 256)
		m_decimation = 256;
#endif
}


float bladeRF_source::sample_rate() {

	return m_sample_rate;

}

int bladeRF_source::tune_dac(int dac) {
	printf("DAC: 0x%.4x\n", dac);
	return bladerf_dac_write(bdev, dac);
}

int bladeRF_source::save_dac(int dac) {
	int rv;
	bool ok;
	bladerf_fpga_size fpga_size;
	struct bladerf_image *image = NULL;
	uint32_t page, count;

	bladerf_get_fpga_size(bdev, &fpga_size);

	image = bladerf_alloc_cal_image(bdev, fpga_size, dac);
	if (!image) {
		return 1;
	}

	rv = bladerf_erase_flash_bytes(bdev, BLADERF_FLASH_ADDR_CAL,
			BLADERF_FLASH_BYTE_LEN_CAL);
	if (rv != 0) {
		return 1;
	}

	rv = bladerf_write_flash_bytes(bdev, image->data, image->address, image->length);

	return 0;
}

int bladeRF_source::tune(double freq) {

	int r;

	pthread_mutex_lock(&m_u_mutex);
	r = bladerf_set_frequency(bdev, BLADERF_MODULE_RX, freq);
	pthread_mutex_unlock(&m_u_mutex);

	return !r;
}


bool bladeRF_source::set_antenna(int antenna) {

	return true;
	//return m_db_rx->select_rx_antenna(antenna);
}


bool bladeRF_source::set_gain(float gain) {

	float min = 0.5, max = 2.0;

	if((gain < 0.0) || (1.0 < gain))
		return false;

	return !bladerf_set_rxvga2(bdev, 3);
	return !bladerf_set_rxvga2(bdev, min + gain * (max - min));
}


/*
 * open() should be called before multiple threads access bladeRF_source.
 */
int bladeRF_source::open(unsigned int subdev) {

	int do_set_decim = 0;

	if(!bdev) {
		int status;

		if (bladerf_open(&bdev, NULL)) {
			printf("Couldn't open bladeRF\n");
			exit(1);
		}

#define DEFAULT_STREAM_XFERS        64
#define DEFAULT_STREAM_BUFFERS      5600
#define DEFAULT_STREAM_SAMPLES      2048
#define DEFAULT_STREAM_TIMEOUT      4000

		status = bladerf_sync_config(bdev,
				static_cast<bladerf_channel_layout>(BLADERF_CHANNEL_RX(0)),
				BLADERF_FORMAT_SC16_Q11,
				DEFAULT_STREAM_BUFFERS,
				DEFAULT_STREAM_SAMPLES,
				DEFAULT_STREAM_XFERS,
				DEFAULT_STREAM_TIMEOUT
				);

		if(!m_decimation) {
			do_set_decim = 1;
			m_decimation = 4;
		}

//		if(do_set_decim) {
//			calculate_decimation();
//		}

		//m_u_rx->set_decim_rate(m_decimation);
//		m_sample_rate = (double)m_u_rx->fpga_master_clock_freq() / m_decimation;
		unsigned int bw;
		bladerf_set_bandwidth(bdev, BLADERF_MODULE_RX, 1500000u, &bw);
		printf("Actual filter bandwidth = %d kHz\n", bw/1000);
		int gain;
		bladerf_set_rxvga1(bdev, 20);
		bladerf_get_rxvga1(bdev, &gain);
		printf("rxvga1 = %d dB\n", gain);
		bladerf_set_rxvga2(bdev, 30);
		bladerf_set_lna_gain(bdev, BLADERF_LNA_GAIN_MAX);

		bladerf_get_rxvga2(bdev, &gain);
		bladerf_dac_write(bdev, 0xa1ea);

		printf("rxvga2 = %d dB\n", gain);
		struct bladerf_rational_rate rate, actual;
		rate.integer = (4 * 13e6) / 48;
		rate.num = (4 * 13e6) - rate.integer * 48;
		rate.den = 48;
		m_sample_rate = (double)4.0 * 13.0e6 / 48.0;

		if (bladerf_set_rational_sample_rate(bdev, BLADERF_MODULE_RX, &rate, &actual)) {
			printf("Error setting RX sampling rate\n");
			return -1;
		}


		if(g_verbosity > 1) {
			fprintf(stderr, "Decimation : %u\n", m_decimation);
			fprintf(stderr, "Sample rate: %f\n", m_sample_rate);
		}
	}
	set_gain(0.45);

	return 0;
}


#define USB_PACKET_SIZE 512

int bladeRF_source::fill(unsigned int num_samples, unsigned int *overrun_i) {

	bool overrun;
	unsigned char ubuf[USB_PACKET_SIZE];
	short *s = (short *)ubuf;
	unsigned int i, j, space, overruns = 0;
	complex *c;

	while((m_cb->data_available() < num_samples) && (m_cb->space_available() > 0)) {

		// read one usb packet from the bladeRF
		pthread_mutex_lock(&m_u_mutex);
		bladerf_sync_rx(bdev, ubuf, 512 / 4, NULL, 0);
		overrun = false;
		pthread_mutex_unlock(&m_u_mutex);
		if(overrun)
			overruns++;

		// write complex<short> input to complex<float> output
		c = (complex *)m_cb->poke(&space);

		// set space to number of complex items to copy
		if(space > (USB_PACKET_SIZE >> 2))
			space = USB_PACKET_SIZE >> 2;

		// write data
		for(i = 0, j = 0; i < space; i += 1, j += 2)
			c[i] = complex(s[j], s[j + 1]);

		// update cb
		m_cb->wrote(i);
	}

	// if the cb is full, we left behind data from the usb packet
	if(m_cb->space_available() == 0) {
		fprintf(stderr, "warning: local overrun\n");
		overruns++;
	}

	if(overrun_i)
		*overrun_i = overruns;

	return 0;
}


int bladeRF_source::read(complex *buf, unsigned int num_samples,
   unsigned int *samples_read) {

	unsigned int n;

	if(fill(num_samples, 0))
		return -1;

	n = m_cb->read(buf, num_samples);

	if(samples_read)
		*samples_read = n;

	return 0;
}


/*
 * Don't hold a lock on this and use the bladeRF at the same time.
 */
circular_buffer *bladeRF_source::get_buffer() {

	return m_cb;
}


int bladeRF_source::flush(unsigned int flush_count) {

	m_cb->flush();
	fill(flush_count * USB_PACKET_SIZE, 0);
	m_cb->flush();

	return 0;
}
