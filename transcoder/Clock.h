#pragma once

#include <chrono>

namespace av
{

/* master clock used by Display to sync streams by timestamp */
class Clock
{
public:
	/* time elapsed in milliseconds since the first clock use */
	uint64_t milliseconds();

	/* returns a value for use by video display to delay frame */
	uint64_t update(uint64_t rts);

	/* invoked from audio callback to adjust correction factor */
	int sync(uint64_t rts);

	/* adjusts correction factor during pause */
	void pause(bool paused);

private:
	std::chrono::steady_clock clock;
	bool started = false;
	uint64_t correction = 0;
	std::chrono::high_resolution_clock::time_point play_start;
	std::chrono::high_resolution_clock::time_point pause_start;

};

}
