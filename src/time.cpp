#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "time.hpp"
#include "sound.hpp"
#include <cstdlib>

#define USDIVIDER 1000000

Time::Time(int delta)
	: current(0)
	, last_dt(0.0f)
	, delta(delta)
	, scale(0)
	, steps(0)
	, paused(true)
	, sound_last_time(0.0)
	, sound(nullptr) {

	set_paused(false);
}

void Time::update(time_t delta){
	move(delta);
}

long Time::update_delta(){
	/* single-stepping */
	if ( steps != 0 ){
		const long int usec = steps * delta;
		steps = 0;
		return usec;
	}

	/* syncing against music */
	if ( sound ){
		const double cur_time = sound->time();
		const long int usec = static_cast<long>(USDIVIDER * (cur_time - sound_last_time));
		sound_last_time = cur_time;
		return usec;
	}

	/* scaled time */
	const float k = (float)scale / 100.0f;
	return static_cast<long>((float)delta * k);
}

void Time::step(int amount){
	paused = true;
	scale = 0;
	steps += amount;
}

void Time::adjust_speed(int amount){
	scale += amount;
	paused = false;
}

int Time::current_scale() const {
	return scale;
}

void Time::toggle_pause(){
	set_paused(!paused);
}

void Time::set_paused(bool state){
	paused = state;
	if ( paused ){
		scale = 0;
	} else if ( scale == 0 ){
		scale = 100;
	}
}

float Time::get() const {
	return (float)current / USDIVIDER;
}

void Time::set(unsigned long steps){
	current = steps * delta;
}

void Time::reset(){
	set(0);
}

long Time::utime() const {
	return current;
}

float Time::dt() const {
	return last_dt;
}

bool Time::sync_to_music(const Sound* m) {
	sound_last_time = m->time();
	if(sound_last_time < 0) {
		//Syncing is not available
		return false;
	}

	sound = m;
	reset();
	return true;
}

void Time::move(long int usec){
	last_dt = (float)usec / USDIVIDER;
	current += usec;
}
