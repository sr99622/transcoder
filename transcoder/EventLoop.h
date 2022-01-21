#pragma once

#include <SDL.h>
#include <string>

#ifdef EOF
#undef EOF

#define SDL_EVENT_LOOP_WAIT 10

namespace av
{

enum EventLoopState {
	PLAY,
	PAUSE,
	SEEK_FORWARD,
	SEEK_REVERSE,
	EOF,
	QUIT
};

#endif EOF



class EventLoop
{
public:
	std::string eventToString(const SDL_Event& event);
	EventLoopState loop();
	SDL_Event last_event;

};

}
