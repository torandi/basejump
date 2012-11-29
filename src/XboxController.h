//#ifndef XBOXCONROLLER_HPP
//#define XBOXCONROLLER_HPP
//
//#include <SDL/SDL.h>
//#include <glm/glm.hpp>
//#include "movable_object.hpp"
//#include "Controller.hpp"
//
//class XboxController : public Controller {
//	public:
//	
//		XboxController();
//		virtual ~XboxController();
//
//		static bool use_joystick; //otherwise mouse
//
//		void parse_event(const SDL_Event &event);
//		
//		enum input_action_t {
//			MOVE_X,
//			MOVE_Y,
//			MOVE_Z,
//			ROTATE_X,
//			ROTATE_Y,
//			ROTATE_Z,
//			ACTION_0,
//			ACTION_1,
//			ACTION_2,
//			ACTION_3,
//			ACTION_4,
//			ACTION_5,
//			ACTION_6,
//			ACTION_7,
//			START,
//			NUM_ACTIONS
//		};
//
//		void reset();
//		void update(float dt); //Should be called last in every frame. Reset temporary values to 0
//
//		float current_value(input_action_t action) const;
//		
//		void update_object(MovableObject &obj, float dt) const;
//
//		glm::vec3 movement_change() const;
//
//		bool button_down(int btn);
//
//		/*
//		 * Combination of has_changed and checking value > 0.9
//		 *
//		 * Simply checks if the button is presed, and was has changed since last check
//		 */
//		bool down(Input::input_action_t action);
//		bool has_changed(input_action_t action, float epsilon) const;
//
//		float normalized_trigger_value(int axis);
//		float normalized_axis_value(int axis);
//		float get_hat_up_down(int hat);
//		float get_hat_right_left(int hat);
//	private:
//		//Sustained are caused by "key down" etc
//		float sustained_values[NUM_ACTIONS];
//		//Temporary are caused by mouse move etc
//		float temporary_values[NUM_ACTIONS];
//		//Used for changed method
//		mutable float previous_value[NUM_ACTIONS];
//
//		bool * moved_triggers;
//
//		SDL_Joystick * joy;
//};
//
//#endif