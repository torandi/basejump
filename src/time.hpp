#ifndef TIME_HPP
#define TIME_HPP

#include "forward.hpp"

#include <time.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

class Time {
public:
	/**
	 * @param delta How many µsec to move when running at 100% speed.
	 */
	Time(int delta);

	/**
	 * Move time forward (or backwards if speed is negative)
	 */
	void update();

	/**
	 * Single step time.
	 * @param amount How many steps to move forward or backward.
	 */
	void step(int amount);

	/**
	 * Change the time speed.
	 * @param amount By how much to adjust.
	 */
	void adjust_speed(int amount);

	/**
	 * Get current timescale.
	 */
	int current_scale() const;

	/**
	 * Toggle pause.
	 */
	void toggle_pause();

	/**
	 * Set paused.
	 */
	void set_paused(bool state);

	/**
	 * Get current time as seconds in floating-point.
	 */
	float get() const;

	/**
	 * Set current time in number of steps.
	 */
	void set(unsigned long steps);

	/**
	 * Reset time to beginning.
	 * Same as set(0)
	 */
	void reset();

	/**
	 * Get current time in µs.
	 */
	long utime() const;

	/**
	 * Get the previous (scaled) delta in seconds.
	 */
	float dt() const;

	/**
	 * Sync time to music.
	 * Returns true if it was possible, false if it failed (eg can't get time data from sound device)
	 */
	bool sync_to_music(const Sound* music);

private:
	void move(long int usec);

	/**
	 * Calculates how many µs has passed since last update.
	 */
	long update_delta();

	long current;
	float last_dt;
	int delta;
	int scale;
	int steps;
	bool paused;
	double sound_last_time;
	const Sound* sound;
};

#endif /* TIME_HPP */
