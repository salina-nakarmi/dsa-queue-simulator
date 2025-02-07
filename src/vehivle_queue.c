#include "queue.h"
#include <string.h>

void initVehicleQueue(VehicleQueue* queue) {
    queue->front = 0;
    queue->rear = -1;
    queue->count = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
}

int enqueueVehicle(VehicleQueue* queue, Vehicle vehicle) {
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->count >= MAX_QUEUE_SIZE) {
        pthread_mutex_unlock(&queue->mutex);
        return 0;  // Queue is full
    }
    
    queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    queue->vehicles[queue->rear] = vehicle;
    queue->count++;
    
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    return 1;
}

int dequeueVehicle(VehicleQueue* queue, Vehicle* vehicle) {
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->count <= 0) {
        pthread_mutex_unlock(&queue->mutex);
        return 0;  // Queue is empty
    }
    
    *vehicle = queue->vehicles[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    queue->count--;
    
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    return 1;
}

int getQueueCount(VehicleQueue* queue) {
    pthread_mutex_lock(&queue->mutex);
    int count = queue->count;
    pthread_mutex_unlock(&queue->mutex);
    return count;
}