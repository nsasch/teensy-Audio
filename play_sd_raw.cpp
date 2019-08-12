/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <Arduino.h>
#include "Audio.h"
#include "play_sd_raw.h"
#include "spi_interrupt.h"


void AudioPlaySdRaw::begin(void)
{
	playing = false;
        should_loop = false;
        keep_preload = false;
	file_offset = 0;
	file_size = 0;
}


bool AudioPlaySdRaw::preload(const char *filename, const bool keep_preload) {
    if (!loadFile(filename)) {
        return false;
    }
    this->keep_preload = keep_preload;
    return true;
}

void AudioPlaySdRaw::closeFile(bool force_cleanup) {
    // force_cleanup ignores the keep_preload flag
    if (keep_preload && !force_cleanup) {
        resetFile();
    } else {
        if (rawfile) {
            rawfile.close();
        }
        #if defined(HAS_KINETIS_SDHC)
                if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
        #else
                AudioStopUsingSPI();
        #endif
    }
}

bool AudioPlaySdRaw::loadFile(const char *filename) {
	stop();

        if (keep_preload) {
            closeFile(true);
        }

#if defined(HAS_KINETIS_SDHC)
	if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStartUsingSPI();
#else
	AudioStartUsingSPI();
#endif
	/* disable interrupts with lower priority than SDHC DMA */
	const uint32_t last_pri { __get_BASEPRI() };
	const uint8_t sdhc_pri { NVIC_GET_PRIORITY(IRQ_SDHC) };
	__set_BASEPRI((sdhc_pri / 16U + 1U) * 16U);
	rawfile = SD.open(filename);
	__set_BASEPRI(last_pri);
	if (!rawfile) {
		//Serial.println("unable to open file");
		#if defined(HAS_KINETIS_SDHC)
			if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
		#else
			AudioStopUsingSPI();
		#endif
		return false;
	}
	file_size = rawfile.size();
	file_offset = 0;
	//Serial.println("able to open file");
	return true;
}

bool AudioPlaySdRaw::play(const char *filename, const bool should_loop)
{
    if (!loadFile(filename)) {
        return false;
    }
    this->keep_preload = false;

    return play(should_loop);
}

bool AudioPlaySdRaw::play(const bool should_loop)
{
    if (!rawfile) {
        // a file must be preloaded
        return false;
    }
    playing = true;
    this->should_loop = should_loop;
    return true;
}

void AudioPlaySdRaw::stop()
{
	__disable_irq();
	if (playing) {
		playing = false;
		__enable_irq();
                closeFile();
	} else {
		__enable_irq();
	}
}

bool AudioPlaySdRaw::resetFile() {
    if (!rawfile.seek(0)) {
        return false;
    }
    file_offset = 0;
    return true;
}

void AudioPlaySdRaw::update(void)
{
	unsigned int i, n;
	audio_block_t *block;

	// only update if we're playing
	if (!playing) return;

	// allocate the audio blocks to transmit
	block = allocate();
	if (block == NULL) return;

	if (rawfile.available() || (should_loop && resetFile() && rawfile.available())) {
		// we can read more data from the file...
		n = rawfile.read(block->data, AUDIO_BLOCK_SAMPLES*2);
		file_offset += n;
		for (i=n/2; i < AUDIO_BLOCK_SAMPLES; i++) {
			block->data[i] = 0;
		}
		transmit(block);
	} else {
            closeFile();
            playing = false;
	}
	release(block);
}

#define B2M (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT / 2.0) // 97352592

uint32_t AudioPlaySdRaw::positionMillis(void)
{
	return ((uint64_t)file_offset * B2M) >> 32;
}

uint32_t AudioPlaySdRaw::lengthMillis(void)
{
	return ((uint64_t)file_size * B2M) >> 32;
}
