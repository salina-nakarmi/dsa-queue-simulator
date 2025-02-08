#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <unistd.h> 
#include <stdio.h> 
#include <string.h>
#include <pthread.h>
#include<arpa/inet.h>
#include "queue.h"

#define MAX_LINE_LENGTH 20
#define MAIN_FONT "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define SCALE 1
#define ROAD_WIDTH 150
#define LANE_WIDTH 50
#define ARROW_SIZE 15

TTF_Font* font =NULL;

//Stores the current and next traffic light states to control road signals--------------------
typedef struct{
    int currentLight;
    int nextLight;
} SharedData;


// Function declarations
bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer);
void drawRoadsAndLane(SDL_Renderer *renderer, TTF_Font *font);
void displayText(SDL_Renderer *renderer, TTF_Font *font, char *text, int x, int y);
void drawLightForB(SDL_Renderer* renderer, bool isRed);
void refreshLight(SDL_Renderer *renderer, SharedData* sharedData);
void* chequeQueue(void* arg);


void updateTrafficLights(int isPriorityActive);
void drawVehiclesInLane(SDL_Renderer* renderer, VehicleQueue* queue, const char* laneName);
// Update this line in your function declarations section
void drawVehicle(SDL_Renderer* renderer, int x, int y, SDL_Color color, const char* lane);
void drawTrafficLight(SDL_Renderer* renderer, VehicleQueue* queue, int x, int y);
void getLanePosition(const char* laneName, int* x, int* y);
int calculateVehicleServing();



// Global queues for each lane
VehicleQueue laneQueues[4];  // A, B, C, D lanes
const char* LANE_NAMES[] = {"AL1", "BL1", "CL1", "DL1"};

//Create a TCP Socket
void* networkReceiverThread(void* arg) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[100] = {0};

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        return NULL;
    }

    //configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(5000);

    // Bind and listen
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return NULL;
    }
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return NULL;
    }

    // Accept a client connection
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        return NULL;
    }

    //reads incoming vehicle data
    while (1) {
        memset(buffer, 0, sizeof(buffer));//clears the buffer before reading
        int bytes_read = read(new_socket, buffer, sizeof(buffer));//reads data into buffer
        if (bytes_read <= 0) break; //if the client disconnets

        // Parse vehicle data
        Vehicle vehicle;
        char lane[4];
        sscanf(buffer, "%[^:]:%s", vehicle.vehicle_number, lane); // vehicle id, lane name
        strcpy(vehicle.lane, lane); //copies the lane name into the vehicle struct

        // Add to appropriate queue
        for (int i = 0; i < 4; i++) {
            if (strcmp(lane, LANE_NAMES[i]) == 0) {
                enqueueVehicle(&laneQueues[i], vehicle);
                break;
            }
        }
    }

    close(new_socket);
    close(server_fd);
    return NULL;
}


void printMessageHelper(const char* message, int count) {
    for (int i = 0; i < count; i++) printf("%s\n", message);
}


//simulator update funcction
void updateSimulation(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

       // Draw roads and lanes
    drawRoadsAndLane(renderer, font);

    // Draw traffic lights for each direction
    for (int i = 0; i < 4; i++) {
        int x = 0, y = 0;
        getLanePosition(LANE_NAMES[i], &x, &y);
        drawTrafficLight(renderer, &laneQueues[i], x - 40, y);  // Offset by 40 pixels
    }

    // Draw vehicles in queues
    for (int i = 0; i < 4; i++) {
        drawVehiclesInLane(renderer, &laneQueues[i], LANE_NAMES[i]);
    }
}

void drawVehiclesInLane(SDL_Renderer* renderer, VehicleQueue* queue, const char* laneName) {
    int x, y;
    getLanePosition(laneName, &x, &y);

    pthread_mutex_lock(&queue->mutex);
    int count = queue->count;
    for (int i = 0; i < count; i++) {
        int index = (queue->front + i) % MAX_QUEUE_SIZE;
        Vehicle vehicle = queue->vehicles[index];
        
        // Calculate position based on lane orientation
        int vehicleX = x;
        int vehicleY = y;
        
        // Adjust spacing based on lane orientation
        if (strncmp(laneName, "A", 1) == 0) {
            vehicleY += i * 50;  // Move down
        } else if (strncmp(laneName, "C", 1) == 0) {
            vehicleY -= i * 50;  // Move up
        } else if (strncmp(laneName, "B", 1) == 0) {
            vehicleX -= i * 50;  // Move left
        } else {  // D lane
            vehicleX += i * 50;  // Move right
        }
        
        int waitTime = time(NULL) - vehicle.arrival_time;
        SDL_Color color = {0, 0, 255, 255}; // Default blue
        
        if (waitTime > 30) color = (SDL_Color){255, 0, 0, 255}; // Red for long wait
        else if (waitTime > 15) color = (SDL_Color){255, 165, 0, 255}; // Orange for medium wait
        
        drawVehicle(renderer, vehicleX, vehicleY, color, laneName);
    }
    pthread_mutex_unlock(&queue->mutex);
}

void drawVehicle(SDL_Renderer* renderer, int x, int y, SDL_Color color, const char* lane) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect vehicle;
    
    // Adjust vehicle orientation based on lane
    if (strncmp(lane, "A", 1) == 0 || strncmp(lane, "C", 1) == 0) {
        // Vertical orientation for A and C lanes
        vehicle = (SDL_Rect){x - 10, y, 20, 40};
    } else {
        // Horizontal orientation for B and D lanes
        vehicle = (SDL_Rect){x, y - 10, 40, 20};
    }
    
    SDL_RenderFillRect(renderer, &vehicle);
}

void drawTrafficLight(SDL_Renderer* renderer, VehicleQueue* queue, int x, int y) {
    SDL_Rect lightBox = {x, y, 30, 60};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &lightBox);
    
    // Draw the current light state
    SDL_Rect light = {x + 5, y + 5, 20, 20};
    if (queue->light_state == STATE_RED) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    }
    SDL_RenderFillRect(renderer, &light);
}


 void updateTrafficLights(int isPriorityActive) {
    static time_t lastUpdate = 0;
    time_t currentTime = time(NULL);
    
    // Update lights every 5 seconds in normal conditions
    if (currentTime - lastUpdate < 5 && !isPriorityActive) {
        return;
    }
    
    // Priority handling for AL2
    if (isPriorityActive) {
        // Keep AL2 green until queue reduces
        for (int i = 0; i < 4; i++) {
            if (strcmp(LANE_NAMES[i], "AL2") == 0) {
                laneQueues[i].light_state = STATE_GREEN;
            } else {
                laneQueues[i].light_state = STATE_RED;
            }
        }
    } else {
        // Normal rotation of lights
        static int currentLane = 0;
        
        // Reset all to red
        for (int i = 0; i < 4; i++) {
            laneQueues[i].light_state = STATE_RED;
        }
        
        // Set current lane to green
        laneQueues[currentLane].light_state = STATE_GREEN;
        
        // Move to next lane
        currentLane = (currentLane + 1) % 4;
    }
    
    lastUpdate = currentTime;
}

int calculateVehicleServing() {
    // Calculate average number of waiting vehicles
    int totalVehicles = 0;
    int normalLanes = 0;
    
    for (int i = 0; i < 4; i++) {
        if (laneQueues[i].priority == 0) {
            totalVehicles += getQueueCount(&laneQueues[i]);
            normalLanes++;
        }
    }
    
    // Calculate vehicles to serve based on formula from assignment
    if (normalLanes > 0) {
        return totalVehicles / normalLanes;
    }
    return 1; // Default to 1 if no normal lanes
}


//helper function
void getLanePosition(const char* laneName, int* x, int* y) {
    // Set starting positions for each lane based on junction layout
    if (strcmp(laneName, "AL1") == 0) {
        *x = WINDOW_WIDTH/2 - LANE_WIDTH/2;  // Center of leftmost lane
        *y = WINDOW_HEIGHT/4;  // Start from top quarter
    } else if (strcmp(laneName, "BL1") == 0) {
        *x = WINDOW_WIDTH/4;  // Start from left quarter
        *y = WINDOW_HEIGHT/2 - LANE_WIDTH/2;  // Center of top lane
    } else if (strcmp(laneName, "CL1") == 0) {
        *x = WINDOW_WIDTH/2 + LANE_WIDTH/2;  // Center of rightmost lane
        *y = WINDOW_HEIGHT*3/4;  // Start from bottom quarter
    } else if (strcmp(laneName, "DL1") == 0) {
        *x = WINDOW_WIDTH*3/4;  // Start from right quarter
        *y = WINDOW_HEIGHT/2 + LANE_WIDTH/2;  // Center of bottom lane
    }
}

int main() {
    // Initialize queues
    for (int i = 0; i < 4; i++) {
        initVehicleQueue(&laneQueues[i]);
    }

    // Create network receiver thread
    pthread_t networkThread;
    if (pthread_create(&networkThread, NULL, networkReceiverThread, NULL) != 0) {
        perror("Failed to create network thread");
        return -1;
    }

    // Initialize SDL and other components as before
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    if (!initializeSDL(&window, &renderer)) {
        return -1;
    }


    font = TTF_OpenFont(MAIN_FONT, 24);  // Add appropriate font size
    if (!font) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        return false;
    }   


    // Main loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Update simulation state
        updateSimulation(renderer);
        
        // Render frame
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}


//Creates an SDL window and renderer.------------------------------------------------
bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }
    // font init
    if (TTF_Init() < 0) {
        SDL_Log("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        return false;
    }


    *window = SDL_CreateWindow("Junction Diagram",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               WINDOW_WIDTH*SCALE, WINDOW_HEIGHT*SCALE,
                               SDL_WINDOW_SHOWN);
    if (!*window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    // if you have high resolution monitor 2K or 4K then scale
    SDL_RenderSetScale(*renderer, SCALE, SCALE);

    if (!*renderer) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(*window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    return true;
}




void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}


void drawArrwow(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, int x3, int y3) {
    // Sort vertices by ascending Y (bubble sort approach)
    if (y1 > y2) { swap(&y1, &y2); swap(&x1, &x2); }
    if (y1 > y3) { swap(&y1, &y3); swap(&x1, &x3); }
    if (y2 > y3) { swap(&y2, &y3); swap(&x2, &x3); }

    // Compute slopes
    float dx1 = (y2 - y1) ? (float)(x2 - x1) / (y2 - y1) : 0;
    float dx2 = (y3 - y1) ? (float)(x3 - x1) / (y3 - y1) : 0;
    float dx3 = (y3 - y2) ? (float)(x3 - x2) / (y3 - y2) : 0;

    float sx1 = x1, sx2 = x1;

    // Fill first part (top to middle)
    for (int y = y1; y < y2; y++) {
        SDL_RenderDrawLine(renderer, (int)sx1, y, (int)sx2, y);
        sx1 += dx1;
        sx2 += dx2;
    }

    sx1 = x2;

    // Fill second part (middle to bottom)
    for (int y = y2; y <= y3; y++) {
        SDL_RenderDrawLine(renderer, (int)sx1, y, (int)sx2, y);
        sx1 += dx3;
        sx2 += dx2;
    }
}


//draws a traffic light with red or green colors.-----------------
void drawLightForB(SDL_Renderer* renderer, bool isRed){
    // draw light box
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_Rect lightBox = {400, 300, 50, 30};
    SDL_RenderFillRect(renderer, &lightBox);
    // draw light
    if(isRed) SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red
    else SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255);    // green
    SDL_Rect straight_Light = {405, 305, 20, 20};
    SDL_RenderFillRect(renderer, &straight_Light);
    drawArrwow(renderer, 435,305, 435, 305+20, 435+10, 305+10);
}


//Draws a crossroad layout with labeled lanes.------------------------
void drawRoadsAndLane(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_SetRenderDrawColor(renderer, 211,211,211,255);
    // Vertical road
    
    SDL_Rect verticalRoad = {WINDOW_WIDTH / 2 - ROAD_WIDTH / 2, 0, ROAD_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &verticalRoad);

    // Horizontal road
    SDL_Rect horizontalRoad = {0, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2, WINDOW_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &horizontalRoad);
    // draw horizontal lanes
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for(int i=0; i<=3; i++){
        // Horizontal lanes
        SDL_RenderDrawLine(renderer, 
            0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + LANE_WIDTH*i,  // x1,y1
            WINDOW_WIDTH/2 - ROAD_WIDTH/2, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + LANE_WIDTH*i // x2, y2
        );
        SDL_RenderDrawLine(renderer, 
            800, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + LANE_WIDTH*i,
            WINDOW_WIDTH/2 + ROAD_WIDTH/2, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + LANE_WIDTH*i
        );
        // Vertical lanes
        SDL_RenderDrawLine(renderer,
            WINDOW_WIDTH/2 - ROAD_WIDTH/2 + LANE_WIDTH*i, 0,
            WINDOW_WIDTH/2 - ROAD_WIDTH/2 + LANE_WIDTH*i, WINDOW_HEIGHT/2 - ROAD_WIDTH/2
        );
        SDL_RenderDrawLine(renderer,
            WINDOW_WIDTH/2 - ROAD_WIDTH/2 + LANE_WIDTH*i, 800,
            WINDOW_WIDTH/2 - ROAD_WIDTH/2 + LANE_WIDTH*i, WINDOW_HEIGHT/2 + ROAD_WIDTH/2
        );
    }
    displayText(renderer, font, "A",400, 10);
    displayText(renderer, font, "B",400,770);
    displayText(renderer, font, "D",10,400);
    displayText(renderer, font, "C",770,400);
    
}


void displayText(SDL_Renderer *renderer, TTF_Font *font, char *text, int x, int y){
    // display necessary text
    SDL_Color textColor = {0, 0, 0, 255}; // black color
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, textColor);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    SDL_Rect textRect = {x,y,0,0 };
    SDL_QueryTexture(texture, NULL, NULL, &textRect.w, &textRect.h);
    SDL_Log("DIM of SDL_Rect %d %d %d %d", textRect.x, textRect.y, textRect.h, textRect.w);
    // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    // SDL_Log("TTF_Error: %s\n", TTF_GetError());
    SDL_RenderCopy(renderer, texture, NULL, &textRect);
    // SDL_Log("TTF_Error: %s\n", TTF_GetError());
}


//Updates traffic light states.-------------------------------------------------------
void refreshLight(SDL_Renderer *renderer, SharedData* sharedData){
    if(sharedData->nextLight == sharedData->currentLight) return; // early return

    if(sharedData->nextLight == 0){ // trun off all lights
        drawLightForB(renderer, false);
    }
    if(sharedData->nextLight == 2) drawLightForB(renderer, true);
    else drawLightForB(renderer, false);
    SDL_RenderPresent(renderer);
    printf("Light of queue updated from %d to %d\n", sharedData->currentLight,  sharedData->nextLight);
    // update the light
    sharedData->currentLight = sharedData->nextLight;
    fflush(stdout);
}


//Switches traffic lights every 5 seconds.---------------------
void* chequeQueue(void* arg){
    SharedData* sharedData = (SharedData*)arg;
    //int i = 1; (not in use right now)
    while (1) {
        sharedData->nextLight = 0;
        sleep(5);
        sharedData->nextLight = 2;
        sleep(5);
    }
}

