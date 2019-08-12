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

#ifndef play_sd_raw_h_
#define play_sd_raw_h_

#include <Arduino.h>     // github.com/PaulStoffregen/cores/blob/master/teensy4/Arduino.h
#include <AudioStream.h> // github.com/PaulStoffregen/cores/blob/master/teensy4/AudioStream.h
#include <SD.h>          // github.com/PaulStoffregen/SD/blob/Juse_Use_SdFat/src/SD.h

class AudioPlaySdRaw : public AudioStream
{
public:
	AudioPlaySdRaw(void) : AudioStream(0, NULL) { begin(); }
	void begin(void);
	bool preload(const char *filename, const bool keep_preload);
	bool play(const char *filename, bool half_sample=false);
        bool play(bool half_sample=false);
	void stop();
	bool isPlaying(void) { return playing; }
	bool isStopped(void) { return !playing; }
        uint32_t getTimeSinceStoppedMs(void) { return time_since_stopped_ms; }
        bool isStoppedForAtLeastMs(uint32_t t) { return isStopped() && time_since_stopped_ms > t; }
	uint32_t positionMillis(void);
	uint32_t lengthMillis(void);
	virtual void update(void);
private:
        void cleanupFile(const bool force_cleanup=false);
	bool loadFile(const char *filename);
	File rawfile;
	uint32_t file_size;
	volatile uint32_t file_offset;
	volatile bool playing;
        bool keep_preload;
        bool half_sample;
        elapsedMillis time_since_stopped_ms;
        alignas(4) uint16_t buffer[512 / 2];
        uint16_t buffer_offset;
        uint16_t buffer_len;
};

#endif
