#include "common.h"
#include "junction.h"

void initJunction(Junction* junction) {
    // Initialize lanes
    for(int i = 0; i < NUM_LANES; i++) {
        junction->lanes[i].front = 0;
        junction->lanes[i].rear = -1;
        junction->lanes[i].count = 0;
        junction->lanes[i].lane_id = 'A' + i;
        junction->lanes[i].is_priority = (i == 0); // Lane A is priority
        
        // Initialize corresponding traffic light
        junction->lights[i].state = 1; // RED
        junction->lights[i].timer = 0;
        junction->lights[i].controlled_lane = 'A' + i;
    }
    
    junction->priority_active = 0;
    junction->current_green = 'A';
}

int addVehicle(Lane* lane, Vehicle vehicle) {
    if(lane->count >= MAX_VEHICLES) {
        return 0; // Lane is full
    }
    
    lane->rear = (lane->rear + 1) % MAX_VEHICLES;
    lane->vehicles[lane->rear] = vehicle;
    lane->count++;
    
    return 1;
}

Vehicle removeVehicle(Lane* lane) {
    Vehicle empty = {0};
    if(lane->count <= 0) {
        return empty;
    }
    
    Vehicle vehicle = lane->vehicles[lane->front];
    lane->front = (lane->front + 1) % MAX_VEHICLES;
    lane->count--;
    
    return vehicle;
}

void updateTrafficState(Junction* junction) {
    // Check priority lane first (Lane A)
    if(junction->lanes[0].count > PRIORITY_THRESHOLD && !junction->priority_active) {
        // Activate priority mode
        junction->priority_active = 1;
        junction->current_green = 'A';
        
        // Set all lights to red except A
        for(int i = 0; i < NUM_LANES; i++) {
            junction->lights[i].state = (i == 0) ? 2 : 1; // GREEN for A, RED for others
        }
        return;
    }
    
    // Reset priority mode if count drops below threshold
    if(junction->priority_active && junction->lanes[0].count < PRIORITY_RESET) {
        junction->priority_active = 0;
    }
    
    // Normal rotation if not in priority mode
    if(!junction->priority_active) {
        // Rotate through lanes
        char next_green = ((junction->current_green - 'A' + 1) % NUM_LANES) + 'A';
        junction->current_green = next_green;
        
        // Update light states
        for(int i = 0; i < NUM_LANES; i++) {
            junction->lights[i].state = (junction->lights[i].controlled_lane == next_green) ? 2 : 1;
        }
    }
}

int isPriorityLaneActive(Junction* junction) {
    return junction->priority_active;
}



