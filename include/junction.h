// include/junction.h
#ifndef JUNCTION_H
#define JUNCTION_H

#include "common.h"

// Function declarations from the junction.c implementation
void initJunction(Junction* junction);
int addVehicle(Lane* lane, Vehicle vehicle);
Vehicle removeVehicle(Lane* lane);
void updateTrafficState(Junction* junction);
int isPriorityLaneActive(Junction* junction);

#endif