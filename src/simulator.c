#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <math.h>
#include "../include/queue.h"

// Window and graphics constants
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define ROAD_WIDTH 150
#define LANE_WIDTH 50
#define VEHICLE_WIDTH 30
#define VEHICLE_HEIGHT 20
#define PORT 8000
#define FONT_SIZE 24
#define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
// Add these declarations to the top of your simulator.c file
#define MAX_MOVING_VEHICLES 10
#define MOVEMENT_SPEED 2  // pixels per frame
#define MAX_WAYPOINTS 3

#define NUM_LANES 12  // A1, B1, C1, D1, AL2, BL2, CL2, DL2, A3, B3, C3, D3, AL2 (priority)

typedef struct {
    Vehicle vehicle;
    int current_x;
    int current_y;
    int waypoints[MAX_WAYPOINTS][2];  // Array of waypoint coordinates [x,y]
    int current_waypoint;
    int total_waypoints;
    bool active;
    int target_x;
    int target_y;
} MovingVehicle;

// Array to store vehicles currently moving through the intersection
MovingVehicle movingVehicles[MAX_MOVING_VEHICLES];

// Global variables
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
VehicleQueue laneQueues[NUM_LANES];  // All lanes
const char* LANE_NAMES[] = {"A1", "B1", "C1", "D1", "A2", "B2", "C2", "D2", "A3", "B3", "C3", "D3"};



// Function declarations
bool initSDL(void);
//void drawText(const char* text, int x, int y);
void drawRoads(void);
void drawVehicle(int x, int y, SDL_Color color, const char* lane);
void drawTrafficLight(int x, int y, int state);
void updateTrafficLights(void);
void processVehicles(void);
void* networkThread(void* arg);
void updateAndRender(void);
void cleanup(void);
// Function prototype
char generateLane();



bool initSDL(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return false;
    }

    window = SDL_CreateWindow("Traffic Junction Simulator",
                           SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_CENTERED,
                           WINDOW_WIDTH, WINDOW_HEIGHT,
                           SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    printf("SDL initialized successfully!\n");
    printf("Window created successfully!\n");
    printf("Renderer created successfully using software rendering!\n");


    font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    if (!font) {
        fprintf(stderr, "Font loading failed: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    return true;
}

// Define the lane relationships
typedef struct {
    const char* source_lane;  // Lane that generates vehicles (A3, B3, etc.)
    const char* target_lane;  // Lane that receives vehicles (A1, B1, etc.)
} LaneMapping;

// Mappings between generating lanes and receiving lanes
const LaneMapping LANE_MAPPINGS[] = {
    //for left turn
    {"A3", "B1"},  // A3 generates vehicles for B1
    {"B3", "C1"},  // B3 generates vehicles for C1
    {"C3", "D1"},  // C3 generates vehicles for D1
    {"D3", "A1"},  // D3 generates vehicles for A1
    //for strainght
    {"A2", "B1"},
    {"B2", "D1"},
    {"C2", "A1"},
    {"D2", "B1"},
    //FOR RIGHT TURN
    {"A2", "D1"},
    {"B2", "A1"},
    {"C2", "B1"},
    {"D2", "C1"},
    
};

// Function definition


void getSourceLanePosition(const char* lane, int* x, int* y, int vehicleIndex) {
    const int spacing = 30; // Space between vehicles
    const int junctionMargin = 150;
    const int centerX = WINDOW_WIDTH / 2;
    const int centerY = WINDOW_HEIGHT / 2;

    // Source Lanes
    if (strcmp(lane, "A2") == 0) {
        *x = centerX + LANE_WIDTH / 2;
        *y = junctionMargin - vehicleIndex * spacing;
    } else if (strcmp(lane, "B2") == 0) {
        *x = WINDOW_WIDTH - junctionMargin + vehicleIndex * spacing;
        *y = centerY + LANE_WIDTH / 2;
    } else if (strcmp(lane, "C2") == 0) {
        *x = centerX - LANE_WIDTH / 2;
        *y = WINDOW_HEIGHT - junctionMargin + vehicleIndex * spacing;
    } else if (strcmp(lane, "D2") == 0) {
        *x = junctionMargin - vehicleIndex * spacing;
        *y = centerY - LANE_WIDTH / 2;
    } else if (strcmp(lane, "A3") == 0) {
        *x = centerX + ROAD_WIDTH / 3.5 + LANE_WIDTH / 3;
        *y = junctionMargin - vehicleIndex * spacing;
    } else if (strcmp(lane, "B3") == 0) {
        *x = WINDOW_WIDTH - junctionMargin + vehicleIndex * spacing;
        *y = centerY + ROAD_WIDTH / 3.5 + LANE_WIDTH / 3;
    } else if (strcmp(lane, "C3") == 0) {
        *x = centerX - ROAD_WIDTH / 3.5 - LANE_WIDTH / 3;
        *y = WINDOW_HEIGHT - junctionMargin + vehicleIndex * spacing;
    } else if (strcmp(lane, "D3") == 0) {
        *x = junctionMargin - vehicleIndex * spacing;
        *y = centerY - ROAD_WIDTH / 3.5 - LANE_WIDTH / 3;
    }
}



void drawRoads(void) {
    // Draw background
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Draw roads
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 128);
    SDL_Rect verticalRoad = {WINDOW_WIDTH/2 - ROAD_WIDTH/2, 0, ROAD_WIDTH, WINDOW_HEIGHT};
    SDL_Rect horizontalRoad = {0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2, WINDOW_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &verticalRoad);
    SDL_RenderFillRect(renderer, &horizontalRoad);

    // Draw source lanes (A3, B3, C3, D3)
    //drawSourceLanes();

    // Draw lane markings for all lanes including the new lanes
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 1; i < 3; i++) {
        // Vertical lane markings
        SDL_RenderDrawLine(renderer,
            WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, 0,
            WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, WINDOW_HEIGHT);

        // Horizontal lane markings
        SDL_RenderDrawLine(renderer,
            0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH,
            WINDOW_WIDTH, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH);
    }
}

void drawVehicle(int x, int y, SDL_Color color, const char* lane) {
    SDL_Rect vehicle;
    
    // Adjust vehicle size and orientation based on lane
    if (strncmp(lane, "A", 1) == 0 || strncmp(lane, "C", 1) == 0) {
        // Vertical vehicles (going north or south)
        vehicle = (SDL_Rect){
            x - VEHICLE_WIDTH/2,
            y - VEHICLE_HEIGHT/2,
            VEHICLE_WIDTH,
            VEHICLE_HEIGHT
        };
    } else {
        // Horizontal vehicles (going east or west)
        vehicle = (SDL_Rect){
            x - VEHICLE_HEIGHT/2,
            y - VEHICLE_WIDTH/2,
            VEHICLE_HEIGHT,
            VEHICLE_WIDTH
        };
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &vehicle);

    // Draw vehicle outline
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &vehicle);
}

void drawTrafficLight(int x, int y, int state) {
    // Draw traffic light housing
    SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
    SDL_Rect housing = {x, y, 30, 60};
    SDL_RenderFillRect(renderer, &housing);

    // Draw light
    SDL_SetRenderDrawColor(renderer,
        state == STATE_RED ? 255 : 40,
        state == STATE_GREEN ? 255 : 40,
        40, 255);
    SDL_Rect light = {x + 5, y + 5, 20, 20};
    SDL_RenderFillRect(renderer, &light);
}

void getLanePosition(const char* lane, int* x, int* y, int vehicleIndex) {
    const int spacing = 30; // Space between vehicles
    const int junctionMargin = 100; // Keep vehicles this far from the junction center
    const int centerX = WINDOW_WIDTH/2; 
    const int centerY = WINDOW_HEIGHT/2;
    
     // Center of each lane (not just offset)
    const int aLaneCenter = centerX - ROAD_WIDTH/2 + LANE_WIDTH/2; // A1 center
    const int bLaneCenter = centerY - ROAD_WIDTH/2 + LANE_WIDTH/2; // B1 center
    const int cLaneCenter = centerX + ROAD_WIDTH/2 - LANE_WIDTH/2; // C1 center
    const int dLaneCenter = centerY + ROAD_WIDTH/2 - LANE_WIDTH/2; // D1 center
    const int a2LaneCenter = centerX + LANE_WIDTH/2; // AL2 center
    const int b2LaneCenter = centerY + LANE_WIDTH/2; // BL2 center
    const int c2LaneCenter = centerX - LANE_WIDTH/2; // CL2 center
    const int d2LaneCenter = centerY - LANE_WIDTH/2; // DL2 center
    const int a3LaneCenter = centerX - ROAD_WIDTH / 2 - LANE_WIDTH / 2; // AL3 center
    const int b3LaneCenter = centerY - ROAD_WIDTH / 2 - LANE_WIDTH / 2; // BL3 center
    const int c3LaneCenter = centerX + ROAD_WIDTH / 2 + LANE_WIDTH / 2; // CL3 center

  if (strcmp(lane, "A1") == 0) {
        *x = aLaneCenter;
        *y = junctionMargin - vehicleIndex * spacing;
    } else if (strcmp(lane, "AL2") == 0) {
        *x = a2LaneCenter;
        *y = junctionMargin - vehicleIndex * spacing;
    } else if (strcmp(lane, "AL3") == 0) {
        *x = a3LaneCenter;
        *y = junctionMargin - vehicleIndex * spacing;
    } else if (strcmp(lane, "B1") == 0) {
        *x = WINDOW_WIDTH - junctionMargin + vehicleIndex * spacing;
        *y = bLaneCenter;
    } else if (strcmp(lane, "BL2") == 0) {
        *x = WINDOW_WIDTH - junctionMargin + vehicleIndex * spacing;
        *y = b2LaneCenter;
    } else if (strcmp(lane, "BL3") == 0) {
        *x = WINDOW_WIDTH - junctionMargin + vehicleIndex * spacing;
        *y = b3LaneCenter;
    } else if (strcmp(lane, "C1") == 0) {
        *x = cLaneCenter;
        *y = WINDOW_HEIGHT - junctionMargin + vehicleIndex * spacing;
    } else if (strcmp(lane, "CL2") == 0) {
        *x = c2LaneCenter;
        *y = WINDOW_HEIGHT - junctionMargin + vehicleIndex * spacing;
    } else if (strcmp(lane, "CL3") == 0) {
        *x = c3LaneCenter;
        *y = WINDOW_HEIGHT - junctionMargin + vehicleIndex * spacing;
    } else if (strcmp(lane, "D1") == 0) {
        *x = junctionMargin - vehicleIndex * spacing;
        *y = dLaneCenter;
    } else if (strcmp(lane, "DL2") == 0) {
        *x = junctionMargin - vehicleIndex * spacing;
        *y = d2LaneCenter;
    }
}
void updateTrafficLights(void) {
    // Constants for priority thresholds
    const int PRIORITY_THRESHOLD = 10;  // A2 gets priority if queue >= 10
    const int PRIORITY_RELEASE = 5;     // Release priority if queue < 5
    const int UPDATE_INTERVAL = 5;      // Update lights every 5 seconds

    static time_t lastUpdate = 0;
    static int currentGreen = 0;        // Index of the current green lane (A2, B2, C2, D2)
    static bool priorityActive = false; // Whether A2 has priority
    time_t now = time(NULL);

    // Only update lights every UPDATE_INTERVAL seconds
    if (now - lastUpdate < UPDATE_INTERVAL) {
        return;
    }
    lastUpdate = now;

    // 1. Check for Priority Mode Activation (A2 is at index 4 in LANE_NAMES)
    if (laneQueues[4].count >= PRIORITY_THRESHOLD) {
        priorityActive = true;
    }

    // 2. Priority Mode for A2
    if (priorityActive) {
        // Set A2 to green and all other controlled lanes to red
        for (int i = 0; i < NUM_LANES; i++) {
            if (strcmp(LANE_NAMES[i], "A2") == 0 || strcmp(LANE_NAMES[i], "B2") == 0 ||
                strcmp(LANE_NAMES[i], "C2") == 0 || strcmp(LANE_NAMES[i], "D2") == 0) {
                laneQueues[i].light_state = (strcmp(LANE_NAMES[i], "A2") == 0) ? STATE_GREEN : STATE_RED;
            }
        }

        // Release Priority if A2 queue drops below the release threshold
        if (laneQueues[4].count < PRIORITY_RELEASE) {
            priorityActive = false;
        }
        return;  // Exit to avoid normal scheduling
    }

    // 3. Normal Mode (Fair Scheduling)
    // Set all controlled lanes to red
    for (int i = 0; i < NUM_LANES; i++) {
        if (strcmp(LANE_NAMES[i], "A2") == 0 || strcmp(LANE_NAMES[i], "B2") == 0 ||
            strcmp(LANE_NAMES[i], "C2") == 0 || strcmp(LANE_NAMES[i], "D2") == 0) {
            laneQueues[i].light_state = STATE_RED;
        }
    }

    // Set the current controlled lane to green
    if (strcmp(LANE_NAMES[currentGreen], "A2") == 0 || strcmp(LANE_NAMES[currentGreen], "B2") == 0 ||
        strcmp(LANE_NAMES[currentGreen], "C2") == 0 || strcmp(LANE_NAMES[currentGreen], "D2") == 0) {
        laneQueues[currentGreen].light_state = STATE_GREEN;
    }

    // Move to the next controlled lane for the next cycle
    currentGreen = (currentGreen + 1) % NUM_LANES;

    // Ensure currentGreen points to a controlled lane (A2, B2, C2, D2)
    while (strcmp(LANE_NAMES[currentGreen], "A2") != 0 && strcmp(LANE_NAMES[currentGreen], "B2") != 0 &&
           strcmp(LANE_NAMES[currentGreen], "C2") != 0 && strcmp(LANE_NAMES[currentGreen], "D2") != 0) {
        currentGreen = (currentGreen + 1) % NUM_LANES;
    }

    // Print the current green lane
    printf("Current green light: Lane %s\n", LANE_NAMES[currentGreen]);
}

// Get destination coordinates based on source lane
void getDestinationCoordinates(const char* sourceLane, int* destX, int* destY) {
    const int centerX = WINDOW_WIDTH/2;
    const int centerY = WINDOW_HEIGHT/2;
    
    // Set destination based on source lane
    if (strcmp(sourceLane, "A1") == 0 || strcmp(sourceLane, "AL2") == 0) {
        // A lanes exit to the bottom
        *destX = centerX - ROAD_WIDTH/2 + LANE_WIDTH/2;
        *destY = WINDOW_HEIGHT;
    } else if (strcmp(sourceLane, "B1") == 0) {
        // B lane exits to the left
        *destX = 0;
        *destY = centerY - ROAD_WIDTH/2 + LANE_WIDTH/2;
    } else if (strcmp(sourceLane, "C1") == 0) {
        // C lane exits to the top
        *destX = centerX + ROAD_WIDTH/2 - LANE_WIDTH/2;
        *destY = 0;
    } else if (strcmp(sourceLane, "D1") == 0) {
        // D lane exits to the right
        *destX = WINDOW_WIDTH;
        *destY = centerY + ROAD_WIDTH/2 - LANE_WIDTH/2;
    }
}

// Add this new function to calculate waypoints for a vehicle's path
void calculateWaypoints(MovingVehicle* movingVehicle) {
    const int centerX = WINDOW_WIDTH / 2;
    const int centerY = WINDOW_HEIGHT / 2;
    const int laneWidth = LANE_WIDTH;
    const int roadWidth = ROAD_WIDTH;

    // Clear previous waypoints
    movingVehicle->current_waypoint = 0;
    movingVehicle->total_waypoints = 0;

    // Calculate waypoints based on source lane
    const char* lane = movingVehicle->vehicle.lane;

    // A3 to B1 (right turn from top-right to right-top)
    if (strcmp(lane, "A3") == 0) {
        // Start position in A3
        movingVehicle->current_x = centerX + laneWidth + laneWidth/2;
        movingVehicle->current_y = centerY - roadWidth/2;
        
        // First waypoint: turn point
        movingVehicle->waypoints[0][0] = centerX + roadWidth/2;
        movingVehicle->waypoints[0][1] = centerY - laneWidth/2;
        
        // Final waypoint: exit on B1
        movingVehicle->waypoints[1][0] = WINDOW_WIDTH;
        movingVehicle->waypoints[1][1] = centerY - laneWidth/2;
        
        movingVehicle->total_waypoints = 2;
    }// B3 to C1 (right turn from bottom-right to right-bottom)
    else if (strcmp(lane, "B3") == 0) {
        // Start position in B3
        movingVehicle->current_x = centerX + roadWidth/2;
        movingVehicle->current_y = centerY + laneWidth + laneWidth/2;
        
        // First waypoint: turn point
        movingVehicle->waypoints[0][0] = centerX + laneWidth/2;
        movingVehicle->waypoints[0][1] = centerY + roadWidth/2;
        
        // Final waypoint: exit on C1
        movingVehicle->waypoints[1][0] = centerX + laneWidth/2;
        movingVehicle->waypoints[1][1] = WINDOW_HEIGHT;
        
        movingVehicle->total_waypoints = 2;
    }
    // C3 to D1 (right turn from bottom-left to left-bottom)
    else if (strcmp(lane, "C3") == 0) {
        // Start position in C3
        movingVehicle->current_x = centerX - laneWidth - laneWidth/2;
        movingVehicle->current_y = centerY + roadWidth/2;
        
        // First waypoint: turn point
        movingVehicle->waypoints[0][0] = centerX - roadWidth/2;
        movingVehicle->waypoints[0][1] = centerY + laneWidth/2;
        
        // Final waypoint: exit on D1
        movingVehicle->waypoints[1][0] = 0;
        movingVehicle->waypoints[1][1] = centerY + laneWidth/2;
        
        movingVehicle->total_waypoints = 2;
    }
    // D3 to A1 (right turn from top-left to left-top)
    else if (strcmp(lane, "D3") == 0) {
        // Start position in D3
        movingVehicle->current_x = centerX - roadWidth/2;
        movingVehicle->current_y = centerY - laneWidth - laneWidth/2;
        
        // First waypoint: turn point
        movingVehicle->waypoints[0][0] = centerX - laneWidth/2;
        movingVehicle->waypoints[0][1] = centerY - roadWidth/2;
        
        // Final waypoint: exit on A1
        movingVehicle->waypoints[1][0] = centerX - laneWidth/2;
        movingVehicle->waypoints[1][1] = 0;
        
        movingVehicle->total_waypoints = 2;
    }
}

// Calculate waypoints for Lane 2 (A2, B2, C2, D2)
void calculateWaypointsForLane2(MovingVehicle* movingVehicle, bool turnRight) {
    const int centerX = WINDOW_WIDTH / 2;
    const int centerY = WINDOW_HEIGHT / 2;
    const int laneWidth = LANE_WIDTH;
    const int roadWidth = ROAD_WIDTH;
    const char* lane = movingVehicle->vehicle.lane;

    // A2 can go to C1 (straight) or B1 (right)
    if (strcmp(lane, "A2") == 0) {
        // Start position in A2
        movingVehicle->current_x = centerX + laneWidth/2;
        movingVehicle->current_y = centerY - roadWidth/2;
        
        if (turnRight) {
            // Right turn to B1
            movingVehicle->waypoints[0][0] = centerX + roadWidth/2;
            movingVehicle->waypoints[0][1] = centerY - laneWidth/2;
            
            // Final waypoint: exit on B1
            movingVehicle->waypoints[1][0] = WINDOW_WIDTH;
            movingVehicle->waypoints[1][1] = centerY - laneWidth/2;
            
            movingVehicle->total_waypoints = 2;
        } else {
            // Go straight to C1
            movingVehicle->waypoints[0][0] = centerX + laneWidth/2;
            movingVehicle->waypoints[0][1] = WINDOW_HEIGHT;
            
            movingVehicle->total_waypoints = 1;
        }
    }
    // B2 can go to D1 (straight) or C1 (right)
    else if (strcmp(lane, "B2") == 0) {
        // Start position in B2
        movingVehicle->current_x = centerX + roadWidth/2;
        movingVehicle->current_y = centerY + laneWidth/2;
        
        if (turnRight) {
            // Right turn to C1
            movingVehicle->waypoints[0][0] = centerX + laneWidth/2;
            movingVehicle->waypoints[0][1] = centerY + roadWidth/2;
            
            // Final waypoint: exit on C1
            movingVehicle->waypoints[1][0] = centerX + laneWidth/2;
            movingVehicle->waypoints[1][1] = WINDOW_HEIGHT;
            
            movingVehicle->total_waypoints = 2;
        } else {
            // Go straight to D1
            movingVehicle->waypoints[0][0] = 0;
            movingVehicle->waypoints[0][1] = centerY + laneWidth/2;
            
            movingVehicle->total_waypoints = 1;
        }
    }
    // C2 can go to A1 (straight) or D1 (right)
    else if (strcmp(lane, "C2") == 0) {
        // Start position in C2
        movingVehicle->current_x = centerX - laneWidth/2;
        movingVehicle->current_y = centerY + roadWidth/2;
        
        if (turnRight) {
            // Right turn to D1
            movingVehicle->waypoints[0][0] = centerX - roadWidth/2;
            movingVehicle->waypoints[0][1] = centerY + laneWidth/2;
            
            // Final waypoint: exit on D1
            movingVehicle->waypoints[1][0] = 0;
            movingVehicle->waypoints[1][1] = centerY + laneWidth/2;
            
            movingVehicle->total_waypoints = 2;
        } else {
            // Go straight to A1
            movingVehicle->waypoints[0][0] = centerX - laneWidth/2;
            movingVehicle->waypoints[0][1] = 0;
            
            movingVehicle->total_waypoints = 1;
        }
    }
    // D2 can go to B1 (straight) or A1 (right)
    else if (strcmp(lane, "D2") == 0) {
        // Start position in D2
        movingVehicle->current_x = centerX - roadWidth/2;
        movingVehicle->current_y = centerY - laneWidth/2;
        
        if (turnRight) {
            // Right turn to A1
            movingVehicle->waypoints[0][0] = centerX - laneWidth/2;
            movingVehicle->waypoints[0][1] = centerY - roadWidth/2;
            
            // Final waypoint: exit on A1
            movingVehicle->waypoints[1][0] = centerX - laneWidth/2;
            movingVehicle->waypoints[1][1] = 0;
            
            movingVehicle->total_waypoints = 2;
        } else {
            // Go straight to B1
            movingVehicle->waypoints[0][0] = WINDOW_WIDTH;
            movingVehicle->waypoints[0][1] = centerY - laneWidth/2;
            
            movingVehicle->total_waypoints = 1;
        }
    }
}



// Add a vehicle to moving vehicles array
void addMovingVehicle(Vehicle* vehicle) {
    for (int i = 0; i < MAX_MOVING_VEHICLES; i++) {
        if (!movingVehicles[i].active) {
            // Find starting position based on lane
            int startX, startY;
            getLanePosition(vehicle->lane, &startX, &startY, 0);
            
            // Find destination based on lane
            int destX, destY;
            getDestinationCoordinates(vehicle->lane, &destX, &destY);
            
            // Set up moving vehicle
            movingVehicles[i].vehicle = *vehicle;
            movingVehicles[i].current_x = startX;
            movingVehicles[i].current_y = startY;
            movingVehicles[i].target_x = destX;
            movingVehicles[i].target_y = destY;
            movingVehicles[i].active = true;

             // Calculate waypoints based on source and target lanes
            if (strstr(vehicle->lane, "L2") != NULL) {
    // L2 lanes can go straight or right - randomly decide
    bool turnRight = (rand() % 2 == 0);
    calculateWaypointsForLane2(&movingVehicles[i], turnRight);
} else {
    // Handle L1 and L3 lanes with standard waypoints
    calculateWaypoints(&movingVehicles[i]);
}
            
            printf("Vehicle %s now moving through intersection from %s\n", 
                   vehicle->vehicle_number, vehicle->lane);
            return;
        }
    }
}





void updateMovingVehicles(void) {
    for (int i = 0; i < MAX_MOVING_VEHICLES; i++) {
        if (movingVehicles[i].active && movingVehicles[i].total_waypoints > 0) {
            // Get the current waypoint
            int target_waypoint = movingVehicles[i].current_waypoint;
            int target_x = movingVehicles[i].waypoints[target_waypoint][0];
            int target_y = movingVehicles[i].waypoints[target_waypoint][1];

            // Calculate the direction vector
            int dx = target_x - movingVehicles[i].current_x;
            int dy = target_y - movingVehicles[i].current_y;
            double length = sqrt(dx * dx + dy * dy);

            // Normalize the direction vector
            if (length > 0) {
                double nx = dx / length;
                double ny = dy / length;

                // Move the vehicle
                movingVehicles[i].current_x += (int)(nx * MOVEMENT_SPEED);
                movingVehicles[i].current_y += (int)(ny * MOVEMENT_SPEED);

                // Check if the vehicle has reached the current waypoint
                if (abs(movingVehicles[i].current_x - target_x) < MOVEMENT_SPEED &&
                    abs(movingVehicles[i].current_y - target_y) < MOVEMENT_SPEED) {
                    movingVehicles[i].current_waypoint++;

                    // If all waypoints are completed, deactivate the vehicle
                    if (movingVehicles[i].current_waypoint >= movingVehicles[i].total_waypoints) {
                        movingVehicles[i].active = false;
                        printf("Vehicle %s exited the intersection\n",
                               movingVehicles[i].vehicle.vehicle_number);
                    }
                }
            }
        }
    }
}



// Draw all moving vehicles
void drawMovingVehicles(void) {
    for (int i = 0; i < MAX_MOVING_VEHICLES; i++) {
        if (movingVehicles[i].active) {
            // Determine color based on lane (you could use the same logic as in drawVehicle)
            SDL_Color color = {0, 255, 0, 255}; // Green for moving vehicles
            
            drawVehicle(
                movingVehicles[i].current_x, 
                movingVehicles[i].current_y, 
                color, 
                movingVehicles[i].vehicle.lane
            );
        }
    }
}

void processVehicles(void) {
    for (int i = 0; i < NUM_LANES; i++) {
        // Handle A3, B3, C3, D3 (bypass traffic lights)
        if (strcmp(LANE_NAMES[i], "A3") == 0 || strcmp(LANE_NAMES[i], "B3") == 0 ||
            strcmp(LANE_NAMES[i], "C3") == 0 || strcmp(LANE_NAMES[i], "D3") == 0) {
            // Continuously dequeue vehicles from these lanes
            while (laneQueues[i].count > 0) {
                Vehicle vehicle;
                if (dequeueVehicle(&laneQueues[i], &vehicle)) {
                    addMovingVehicle(&vehicle);
                    printf("Vehicle %s dequeued from lane %s (no traffic light)\n",
                           vehicle.vehicle_number, vehicle.lane);
                }
            }
        }
        // Handle A2, B2, C2, D2 (follow traffic lights)
        else if (strcmp(LANE_NAMES[i], "A2") == 0 || strcmp(LANE_NAMES[i], "B2") == 0 ||
                 strcmp(LANE_NAMES[i], "C2") == 0 || strcmp(LANE_NAMES[i], "D2") == 0) {
            if (laneQueues[i].light_state == STATE_GREEN && laneQueues[i].count > 0) {
                Vehicle vehicle;
                if (dequeueVehicle(&laneQueues[i], &vehicle)) {
                    addMovingVehicle(&vehicle);
                    printf("Vehicle %s dequeued from lane %s (green light)\n",
                           vehicle.vehicle_number, vehicle.lane);
                }
            }
        }
    }
}

// Update networkThread to handle A2, B2, C2, and D2
void* networkThread(void* arg) {
    (void)arg;
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[MAX_VEHICLE_LENGTH + MAX_LANE_LENGTH + 2] = {0}; // +2 for ':' and null terminator

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        return NULL;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        return NULL;
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected\n");

        while (1) {
            int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0) break;

            buffer[bytes_read] = '\0';
            Vehicle vehicle;
            char lane[4];

            if (sscanf(buffer, "%[^:]:%s", vehicle.vehicle_number, lane) == 2) {
                strcpy(vehicle.lane, lane);
                vehicle.arrival_time = time(NULL);

                // Add vehicle to the appropriate lane queue
                for (int i = 0; i < NUM_LANES; i++) {
                    if (strcmp(lane, LANE_NAMES[i]) == 0) {
                        enqueueVehicle(&laneQueues[i], vehicle);
                        printf("Vehicle %s added to lane %s\n", vehicle.vehicle_number, lane);
                        break;
                    }
                }
            }
        }

        close(client_fd);
        printf("Client disconnected\n");
    }
}

void updateAndRender(void) {
    // Define standard positioning variables
    const int centerX = WINDOW_WIDTH/2;  // 400 for your 800x800 window
    const int centerY = WINDOW_HEIGHT/2; // 400 for your 800x800 window
    const int laneOffset = LANE_WIDTH/2; // 25 for your 50px lanes
    
    drawRoads();

      // Update moving vehicles
    updateMovingVehicles();

    // Update and draw traffic lights
    updateTrafficLights();
    for (int i = 0; i < NUM_LANES; i++) {
        // Calculate traffic light positions based on lane
        int light_x = 0, light_y = 0;
        
        if (strncmp(LANE_NAMES[i], "A", 1) == 0) {
            light_x = centerX - laneOffset; // At lane position
            light_y = centerY - ROAD_WIDTH/2 - 20; // Above the junction
        } else if (strncmp(LANE_NAMES[i], "B", 1) == 0) {
            light_x = centerX + ROAD_WIDTH/2 + 20; // Right of the junction
            light_y = centerY - laneOffset; // At lane height
        } else if (strncmp(LANE_NAMES[i], "C", 1) == 0) {
            light_x = centerX + laneOffset; // At lane position
            light_y = centerY + ROAD_WIDTH/2 + 20; // Below the junction
        } else if (strncmp(LANE_NAMES[i], "D", 1) == 0) {
            light_x = centerX - ROAD_WIDTH/2 - 20; // Left of the junction
            light_y = centerY + laneOffset; // At lane height
        }

        drawTrafficLight(light_x, light_y, laneQueues[i].light_state);
    }

    // Draw vehicles in source lanes
    for (int i = 5; i < NUM_LANES; i++) {  // Starting from index 5 (A3, B3, C3, D3)
        pthread_mutex_lock(&laneQueues[i].mutex);
        
        for (int j = 0; j < laneQueues[i].count; j++) {
            int idx = (laneQueues[i].front + j) % MAX_QUEUE_SIZE;
            Vehicle vehicle = laneQueues[i].vehicles[idx];
            
            // Use a different color for source lane vehicles
            SDL_Color color = {255, 255, 0, 255}; // Yellow for source lanes
            
            int x, y;
            getSourceLanePosition(vehicle.lane, &x, &y, j);
            drawVehicle(x, y, color, vehicle.lane);
        }
        
        pthread_mutex_unlock(&laneQueues[i].mutex);
    }

        // Draw vehicles that are moving through the intersection
    drawMovingVehicles();
    
    // Process vehicles (now adds them to moving array instead of just removing)
    processVehicles();

   
    
    SDL_RenderPresent(renderer);
}

void cleanup(void) {
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

int main(void) {
    // Initialize SDL and queues
    if (!initSDL()) {
        return 1;
    }

    // Initialize queues
    for (int i = 0; i < NUM_LANES; i++) {
        initVehicleQueue(&laneQueues[i]);
    }

    // Start network thread
    pthread_t network_thread;
    if (pthread_create(&network_thread, NULL, networkThread, NULL) != 0) {
        perror("Failed to create network SDL_Rethread");
        cleanup();
        return 1;
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

        updateAndRender();
        SDL_Delay(16); // Cap at ~60 FPS
    }

    cleanup();
    return 0;
}
