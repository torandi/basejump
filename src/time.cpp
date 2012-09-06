#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "time.hpp"
#include "music.hpp"
#include <cstdlib>

#define USDIVIDER 1000000

Time::Time(int delta)
	: current(0)
	, prev(0.0f)
	, delta(delta)
	, scale(0)
	, steps(0)
	, paused(true)
	, music(nullptr) {

}

void Time::update(){
	/* single-stepping */
	if ( steps != 0 ){
		const long int usec = steps * delta;
		steps = 0;
		move(usec);
		return;
	}

	if(music == nullptr) {
		/* normal flow */
		const float k = (float)scale / 100.0f;
		const long int usec = delta * k;
		move(usec);
	} else {
		double cur_time = music->time();
		const long int usec = USDIVIDER * (cur_time - music_last_time);
		music_last_time = cur_time;
		move(usec);
	}
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
	return prev;
}

bool Time::sync_to_music(const Music * m) {
	music_last_time = m->time();
	if(music_last_time < 0) {
		//Syncing is not available
		return false;
	} else {
		music = m;
		reset();
		return true;
	}
}

void Time::move(long int usec){
	prev = (float)usec / USDIVIDER;
	current += usec;
}
