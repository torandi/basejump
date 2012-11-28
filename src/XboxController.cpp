//#include "XboxController.h"
//
//XboxController::XboxController()
//{
//	SDL_JoystickEventState(SDL_TRUE);
//	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
//	for(int i=0; i<NUM_ACTIONS; ++i) {
//		sustained_values[i] = 0.0;
//		temporary_values[i] = 0.0;
//		previous_value[i] = -2.0;
//	}
//	if(SDL_NumJoysticks()>0){
//		joy=SDL_JoystickOpen(0);
//		moved_triggers = new bool[SDL_JoystickNumAxes(joy)];
//		for(int i=0; i < SDL_JoystickNumAxes(joy); ++i)
//			moved_triggers[i]=false;
//	}
//}