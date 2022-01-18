#include "Clock.h"
#include <iostream>

uint64_t av::Clock::milliseconds()
{
	if (!started) {
		play_start = clock.now();
		started = true;
	}
	auto current = clock.now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(current - play_start).count();
}

uint64_t av::Clock::update(uint64_t rts)
{
	if (!started) {
		play_start = clock.now();
		started = true;
	}

	uint64_t current = milliseconds() - correction;

	if (current > rts)
		return 0;
	else
		return rts - current;
}

int av::Clock::sync(uint64_t rts)
{
	if (!started) {
		play_start = clock.now();
		started = true;
	}

	uint64_t current = milliseconds() - correction;
	int diff = rts - current;
	correction -= diff;
	return diff;
}

void av::Clock::pause(bool paused)
{
	if (paused) {
		pause_start = clock.now();
	}
	else {
		auto current = clock.now();
		correction += std::chrono::duration_cast<std::chrono::milliseconds>(current - pause_start).count();
	}
}