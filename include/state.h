#include <string>
#include <SDL3/SDL_events.h>

class State
{
public:
	void init();
	void handleEvent(SDL_Event e);
	void update();
	void destroy();
	void postBuffer();
	std::string getName();
};