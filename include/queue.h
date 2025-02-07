#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

#define MAX_QUEUE_SIZE 100
#define MAX_VEHICLE_LENGTH 15
#define MAX_LANE_LENGTH 4
#define MAX_VEHICLE_SIZE 20

#define STATE_RED 1
#define STATE_GREEN 2

typedef struct {
    char vehicle_number[MAX_VEHICLE_SIZE];
    char lane[MAX_LANE_LENGTH];
    int arrival_time;
} Vehicle;

typedef struct {
    Vehicle vehicles[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int count;
    int priority;
    int light_state;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} VehicleQueue;


//queue operations
void initVehicleQueue(VehicleQueue* queue);
int enqueueVehicle(VehicleQueue* queue, Vehicle vehicle);
int enqueueVehicle(VehicleQueue* queue, Vehicle* vehicle);
int getQueueCount(VehicleQueue* queue);
void updateQueuePriority(VehicleQueue* queue);

#endif