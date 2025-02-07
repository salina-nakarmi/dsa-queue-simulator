#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

#define MAX_QUEUE_SIZE 100
#define MAX_VEHICLE_LENGTH 15
#define MAX_LANE_LENGTH 4

typedef struct {
    char vehicle_number[MAX_VEHICLE_SIZE];
    char lane[MAX_LANE_LENGTH];
} Vehicle;

typedef struct {
    Vehicle vehicles[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} VehicleQueue;


//queue operations
void initVehicleQueue(VehicleQueue* queue);
int enqueueVehicle(VehicleQueue* queue, Vehicle vehicle);
int enqueueVehicle(VehicleQueue* queue, Vehicle* vehicle);
int getQueueCount(VehicleQueue* queue);

#endif