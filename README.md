# System Programming Team Project
Real-time Line tracing & QR recognition

## Team name : 22조

201911012 공영재 /
202011037 김민재 /
202011094 박현우 /
202211068 류경찬

## Rules
On the 5 by 5 chessboard board, a game in which you face your opponent and get a high score for eating items with different scores dynamically generated at random on the map. At the same time, each player can set up a bomb while playing.

## Code details
In the qr_recognition code, the qr value is dynamically written as a text file, received from the algorithm code, transmitted and received to the server, the shortest distance direction is determined, and the information is written in the text file. Map information is received in real time through threading, and data is shared as an output text file between each code. After that, the corresponding direction information is imported from the line tracking code and then driven according to the direction.

## Sample video
![KakaoTalk_20240701_233128354-ezgif com-optimize (1)](https://github.com/Yeongjae-Kong/system_programming/assets/67358433/f74e477c-0476-4796-a14a-b42caebdb164)

<!-- HOW TO RUN-->

## How to compile
- make

## How to run?
```
# demo_code directory
# do this after opening the server.
./qrrec &
./algorithm &
./linetracer &
```

<!-- HOW TO RUN Server-->
# (2024_SystemProgramming_Server)
## How to compile?
- make
- You will get an executable binary "Server"
## How to run?
- ./Server portnumber
- Ex) ./Server 8080
## How to adapt noimshow.so
- Load the noimshow.so file using LD_PRELOAD before the raspbot executable command.
- EX) LD_PRELOAD=./noimshow.so ./Client 127.0.0.1 8000
- Client is the name of our Raspbot executable file

#### ** Winning starategy

1. First, get the map from the server, and then find the coordinates of the item closest to the current location with the determine Direction() function through post-processing.
2. If there are two or more items with the same distance, the score finds the coordinates of the largest value among them.
3. Declare the direction the car is looking at through the currentDir variable of the code. After that, consider my coordinates and the direction the car is looking at, find the direction to go to the target coordinates. (When the direction of movement is determined, the direction the car looks at changes according to the direction of rotation, so the currentDir is also changed.)
4. At this time, if the straight line is possible at the shortest distance to targetItem, a high-speed straight line should be prioritized, and if it is impossible to go straight, consider left or right, and finally reverse. When determining the direction, check whether it is out of the size of the map or whether there is a trap in the direction to go. At this time, the priority of left and right turns is to move to a closer path, taking into account the difference in rows and columns to targetItem. If all three directions, left, right, and straight are out of the map or have a trap installed, reverse.
5. In the case of a bomb, it was assumed that the opponent had a high probability of constructing the algorithm first to go straight. Later, a strategy was used to block the opponent with the wall when going to the corner to cause significant losses to teams that did not consider the trap or did not implement the backward movement in the conditional statement. In other words, it was implemented to be trapped when going in a straight direction depending on the opponent's starting position. In this way, when the opponent's car goes to the corner, it becomes trapped in the bomb and wall, and if the opponent does not implement the backward movement, it cannot escape. However, given the opportunity cost, installing a bomb was not prioritized, but our car going to eat points was prioritized. Therefore, on the road to the item, if the coordinates of the bomb were set, the bomb was installed.
