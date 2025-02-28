# Traffic Queue Simulator

A real-time traffic(self generated vehicles) intersection simulation that simulates traffic flow at a four-way intersection with traffic lights and multiple lanes. The simulation demonstrates vehicle movement, lane changing, and traffic light management for optimal traffic flow.

## DEMO
![Traffic Simulation Demo](./assets/images/traffic.gif)

## Features
- Real-time traffic simulation at a 4-way intersection

- Random vehicle generation

- Dynamic traffic light system

- Queue-based traffic management

- Priority handling for Lane A2

- Vehicle turning mechanics

- Three lanes per road with distinct behaviors:

   - Lane 1: Incoming lane (vehicles enter here)

   - Lane 2: Follows traffic lights for dequeuing

   - Lane 3: Dequeues vehicles directly to the next lane without traffic light dependency

## Logic

# Lane Behavior
- Lane A3, B3, C3, D3:

Dequeue vehicles directly to their respective lanes (B1, C1, D1, A1) without needing traffic lights.

- Lane A2, B2, C2, D2:

Follow traffic lights for dequeuing:

A2 → C1, D1

B2 → D1, A1

C2 → B1, A1

D2 → B1, C1

- Lane 1:

Acts as the incoming lane where vehicles are enqueued.

# Priority Handling
- Lane A2 is the priority lane:

When the vehicle count reaches 5, this lane is given high priority and is dequeued first.

# Other lanes:

Vehicles are dequeued when the count exceeds 10, and dequeuing continues until the count drops to 5.


## Prerequisites:
Before running this project, make sure you have the following installed:

- SDL2 Library
- C Compiler (GCC recommended)
- MinGW (for Windows)


## Building and Running
<pre>make clean && make && make run</pre>


## References:
SDL2 Documentation: https://wiki.libsdl.org/SDL2/FrontPage

SDL2_ttf Documentation:https://github.com/libsdl-org/SDL_ttf