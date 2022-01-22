#include "defines.h"
#include <sys/time.h>

Timer timer;

extern SearchData sd;

extern bool time_over;

// (In)formally starts a search
void go() {
	time_over = false;
	task = TASK_SEARCH;
}

// Decrements move_time, and if it's within 500
// sets it to -1 (shuts it all down)
void moveTime() {
	if (timer.move_time > 500) {
		sd.move_time = timer.move_time - 500;
	} else {
		sd.move_time = -1;
	}
}

// Checks if time_over, if there's any new commands, 
// if search has finished, and finally if we've exceeded move_time
bool timeCheckRoot() {
	if (time_over)
		return true;

	input();
	if (task == TASK_NOTHING)
		return true;

	return ((int)(getTime() - sd.start_time) > sd.move_time);
}

// Called every thousand nodes or so
// Otherwise similar to timeCheckRoot()
bool timeCheck() {
	if (sd.depth <= 1)
		return false;

	input();
	if (task == TASK_NOTHING)
		return true;

	return ((int)(getTime() - sd.start_time) > sd.move_time);
}

// Utility that gets linux time of day
unsigned int getTime() {
	timeval t;
	gettimeofday(&t, 0);
	return t.tv_usec;
}
