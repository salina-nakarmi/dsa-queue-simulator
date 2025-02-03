#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_VEHICLES 100        //max veh per line
#define NUM_LANES 4             //no of lanes(A, B, C, D)
#define PRIORITY_THRESHOLD 10   //veh required to activate priority
#define PRIORITY_RESET 5        //veh required to reset prioprity

// Vehicle structure
typedef struct {
    char number[9];        // Vehicle number (e.g., "AB1CD234")
    char lane;            // Lane identifier (A, B, C, D)
    int lane_number;      // Lane number (1 or 2)
    time_t arrival_time;  // Arrival timestamp
    int is_waiting;       // Waiting status
} Vehicle;

// Lane structure
typedef struct {
    Vehicle vehicles[MAX_VEHICLES]; //Array of vehicles in lane
    int front;                      //Point to the first vehicle
    int rear;                       //Points to last vehicle
    int count;                      //no of vehicels in the lane
    int is_priority;                //1 if lane is prioritized
    char lane_id;                   //lane identifier(A, B, C, D)
} Lane;

// Traffic Light structure
typedef struct {
    int state;          // 1: RED, 2: GREEN
    int timer;          //countdown time for time change
    char controlled_lane;//Lane that the traffic light controls(A, B, C, D)
} TrafficLight;

// Junction state
typedef struct {
    Lane lanes[NUM_LANES];          //Array of lanes (A,B,C,D)
    TrafficLight lights[NUM_LANES]; //Traffic lights for each lane
    int priority_active;            //1 if priority lane is active
    char current_green;             //lane currently allowed to pass
} Junction;

// Function declarations
void initJunction(Junction* junction);
int addVehicle(Lane* lane, Vehicle vehicle);
Vehicle removeVehicle(Lane* lane);
void updateTrafficState(Junction* junction);
int isPriorityLaneActive(Junction* junction);

#endif