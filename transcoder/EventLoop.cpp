#include "EventLoop.h"
#include <iostream>
#include <sstream>
#include <thread>

av::EventLoopState av::EventLoop::loop()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			//std::cout << eventToString(event) << std::endl;
			return EventLoopState::QUIT;
		}
		else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym == SDLK_ESCAPE) {
				//std::cout << eventToString(event) << std::endl;
				return EventLoopState::QUIT;
			}
			else if (event.key.keysym.sym == SDLK_SPACE) {
				//std::cout << eventToString(event) << std::endl;
				return EventLoopState::PAUSE;
			}
		}
		//SDL_Delay(1);
	}
	return EventLoopState::PLAY;
}






























































std::string av::EventLoop::eventToString(const SDL_Event& event)
{
	std::stringstream str;
	switch (event.type) {
	case SDL_FIRSTEVENT:
		str << "SDL_FIRSTEVENT";
		break;
	case SDL_QUIT:
		str << "SDL_QUIT";
		break;
	case SDL_APP_TERMINATING:
		str << "SDL_APP_TERMINATING";
		break;
	case SDL_APP_LOWMEMORY:
		str << "SDL_APP_LOWMEMORY";
		break;
	case SDL_APP_WILLENTERBACKGROUND:
		str << "SDL_APP_WILLENTERBACKGROUND";
		break;
	case SDL_APP_DIDENTERBACKGROUND:
		str << "SDL_APP_DIDENTERBACKGROUND";
		break;
	case SDL_APP_WILLENTERFOREGROUND:
		str << "SDL_APP_WILLENTERFOREGROUND";
		break;
	case SDL_APP_DIDENTERFOREGROUND:
		str << "SDL_APP_DIDENTERFOREGROUND";
		break;
	case SDL_WINDOWEVENT:
		str << "SDL_WINDOWEVENT";
		SDL_WindowEvent w = event.window;
		switch (event.window.event) {
		case SDL_WINDOWEVENT_SHOWN:
			str << " Window " << event.window.windowID << " shown";
			break;
		case SDL_WINDOWEVENT_HIDDEN:
			str << " Window " << event.window.windowID << " hidden";
			break;
		case SDL_WINDOWEVENT_EXPOSED:
			str << " Window " << event.window.windowID << " exposed";
			break;
		case SDL_WINDOWEVENT_MOVED:
			str << " Window " << event.window.windowID << " moved to " << event.window.data1 << ", " << event.window.data2;
			break;
		case SDL_WINDOWEVENT_RESIZED:
			str << " Window " << event.window.windowID << " resized " << event.window.data1 << " x " << event.window.data2;
			break;
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			str << " Window " << event.window.windowID << " size changed to" << event.window.data1 << " x " << event.window.data2;
			break;
		case SDL_WINDOWEVENT_MINIMIZED:
			str << " Window " << event.window.windowID << " minimized";
			break;
		case SDL_WINDOWEVENT_MAXIMIZED:
			str << " Window" << event.window.windowID <<  " maximized";
			break;
		case SDL_WINDOWEVENT_RESTORED:
			str << " Window " << event.window.windowID << " restored";
			break;
		case SDL_WINDOWEVENT_ENTER:
			str << " Mouse entered window " << event.window.windowID;
			break;
		case SDL_WINDOWEVENT_LEAVE:
			str << " Mouse left window " << event.window.windowID;
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			str << " Window " << event.window.windowID << " gained keyboard focus";
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			str << " Window " << event.window.windowID << " lost keyboard focus";
			break;
		case SDL_WINDOWEVENT_CLOSE:
			str << " Window " << event.window.windowID << " closed" << event.window.windowID;
			break;
		case SDL_WINDOWEVENT_TAKE_FOCUS:
			str << " Window " << event.window.windowID << " is offered a focus" << event.window.windowID;
			break;
		case SDL_WINDOWEVENT_HIT_TEST:
			str << " Window " << event.window.windowID << " has a special hit test";
			break;
		default:
			str << " Window " << event.window.windowID << " got unknown event" << event.window.event;
			break;
		}
		break;
	case SDL_SYSWMEVENT:
		str << "SDL_SYSWMEVENT";
		break;
	case SDL_KEYDOWN:
		str << "SDL_KEYDOWN";
		break;
	case SDL_KEYUP:
		str << "SDL_KEYUP";
		break;
	case SDL_TEXTEDITING:
		str << "SDL_TEXTEDITING";
		break;
	case SDL_TEXTINPUT:
		str << "SDL_TEXTINPUT";
		break;
	case SDL_KEYMAPCHANGED:
		str << "SDL_KEYMAPCHANGED";
		break;
	case SDL_MOUSEMOTION:
		str << "SDL_MOUSEMOTION";
		break;
	case SDL_MOUSEBUTTONDOWN:
		str << "SDL_MOUSEBUTTONDOWN";
		break;
	case SDL_MOUSEBUTTONUP:
		str << "SDL_MOUSEBUTTONUP";
		break;
	case SDL_MOUSEWHEEL:
		str << "SDL_MOUSEWHEEL";
		break;
	case SDL_JOYAXISMOTION:
		str << "SDL_JOYAXISMOTION";
		break;
	case SDL_JOYBALLMOTION:
		str << "SDL_JOYBALLMOTION";
		break;
	case SDL_JOYHATMOTION:
		str << "SDL_JOYHATMOTION";
		break;
	case SDL_JOYBUTTONUP:
		str << "SDL_JOYBUTTONUP";
		break;
	case SDL_JOYDEVICEADDED:
		str << "SDL_JOYDEVICEADDED";
		break;
	case SDL_JOYDEVICEREMOVED:
		str << "SDL_JOYDEVICEREMOVED";
		break;
	case SDL_CONTROLLERAXISMOTION:
		str << "SDL_CONTROLLERAXISMOTION";
		break;
	case SDL_CONTROLLERBUTTONDOWN:
		str << "SDL_CONTROLLERBUTTONDOWN";
		break;
	case SDL_CONTROLLERBUTTONUP:
		str << "SDL_CONTROLLERBUTTONUP";
		break;
	case SDL_CONTROLLERDEVICEADDED:
		str << "SDL_CONTROLLERDEVICEADDED";
		break;
	case SDL_CONTROLLERDEVICEREMOVED:
		str << "SDL_CONTROLLERDEVICEREMOVED";
		break;
	case SDL_CONTROLLERDEVICEREMAPPED:
		str << "SDL_CONTROLLERDEVICEREMAPPED";
		break;
	case SDL_FINGERDOWN:
		str << "SDL_FINGERDOWN";
		break;
	case SDL_FINGERUP:
		str << "SDL_FINGERUP";
		break;
	case SDL_FINGERMOTION:
		str << "SDL_FINGERMOTION";
		break;
	case SDL_DOLLARGESTURE:
		str << "SDL_DOLLARGESTURE";
		break;
	case SDL_DOLLARRECORD:
		str << "SDL_DOLLARRECORD";
		break;
	case SDL_MULTIGESTURE:
		str << "SDL_MULTIGESTURE";
		break;
	case SDL_CLIPBOARDUPDATE:
		str << "SDL_CLIPBOARDUPDATE";
		break;
	case SDL_DROPFILE:
		str << "SDL_DROPFILE";
		break;
	case SDL_DROPTEXT:
		str << "SDL_DROPTEXT";
		break;
	case SDL_DROPBEGIN:
		str << "SDL_DROPBEGIN";
		break;
	case SDL_DROPCOMPLETE:
		str << "SDL_DROPCOMPLETE";
		break;
	case SDL_AUDIODEVICEADDED:
		str << "SDL_AUDIODEVICEADDED";
		if (event.adevice.iscapture)
			str << " Input Device: ";
		else
			str << " Output Device: ";
		str << SDL_GetAudioDeviceName(event.adevice.which, event.adevice.iscapture);
		break;
	case SDL_AUDIODEVICEREMOVED:
		str << "SDL_AUDIODEVICEREMOVED";
		break;
	case SDL_RENDER_TARGETS_RESET:
		str << "SDL_RENDER_TARGETS_RESET";
		break;
	case SDL_RENDER_DEVICE_RESET:
		str << "SDL_RENDER_DEVICE_RESET";
		break;
	case SDL_USEREVENT:
		str << "SDL_USEREVENT";
		break;
	case SDL_LASTEVENT:
		str << "SDL_LASTEVENT";
		break;
	default:
		str << "EVENT TYPE NOT FOUND";
	}

	return str.str();
}