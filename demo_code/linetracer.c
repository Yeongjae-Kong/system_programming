#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <wiringPi.h>
#include <string.h>
#include <pthread.h>


// Define the YB_Pcb_Car struct
typedef struct {
    int file;
    int addr;
} YB_Pcb_Car;

char currentCommand[10] = ""; // Current command
pthread_mutex_t commandMutex = PTHREAD_MUTEX_INITIALIZER;

// Function to get the i2c device
int get_i2c_device(YB_Pcb_Car* car, int address, int i2c_bus) {
    char filename[20];
    snprintf(filename, 19, "/dev/i2c-%d", i2c_bus);
    car->file = open(filename, O_RDWR);
    if (car->file < 0) {
        perror("Failed to open the i2c bus");
        return -1;
    }
    if (ioctl(car->file, I2C_SLAVE, address) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        close(car->file);
        return -1;
    }
    return car->file;
}

// Function to write a single byte to the I2C device
void write_u8(YB_Pcb_Car* car, int reg, uint8_t data) {
    uint8_t buffer[2] = { (uint8_t)reg, data };
    if (write(car->file, buffer, 2) != 2) {
        perror("write_u8 I2C error");
    }
}

// Function to write a register
void write_reg(YB_Pcb_Car* car, int reg) {
    uint8_t buffer[1] = { (uint8_t)reg };
    if (write(car->file, buffer, 1) != 1) {
        perror("write_reg I2C error");
    }
}

// Function to write an array of data to the I2C device
void write_array(YB_Pcb_Car* car, int reg, uint8_t* data, size_t length) {
    uint8_t* buffer = (uint8_t*)malloc(length + 1);
    if (buffer == NULL) {
        perror("Failed to allocate memory");
        return;
    }
    buffer[0] = (uint8_t)reg;
    for (size_t i = 0; i < length; ++i) {
        buffer[i + 1] = data[i];
    }
    if (write(car->file, buffer, length + 1) != length + 1) {
        perror("write_array I2C error");
    }
    free(buffer);
}

// Function to control the car
void Ctrl_Car(YB_Pcb_Car* car, int l_dir, int l_speed, int r_dir, int r_speed) {
    uint8_t data[4] = { (uint8_t)l_dir, (uint8_t)l_speed, (uint8_t)r_dir, (uint8_t)r_speed };
    write_array(car, 0x01, data, 4);
}

// Function to control the car's speed and direction
void Control_Car(YB_Pcb_Car* car, int speed1, int speed2) {
    int dir1 = (speed1 < 0) ? 0 : 1;
    int dir2 = (speed2 < 0) ? 0 : 1;
    Ctrl_Car(car, dir1, abs(speed1), dir2, abs(speed2));
}

// Function to make the car run
void Car_Run(YB_Pcb_Car* car, int speed1, int speed2) {
    Ctrl_Car(car, 1, speed1, 1, speed2);
}

// Function to stop the car
void Car_Stop(YB_Pcb_Car* car) {
    write_u8(car, 0x02, 0x00);
}

// Function to make the car go backwards
void Car_Back(YB_Pcb_Car* car, int speed1, int speed2) {
    Ctrl_Car(car, 0, speed1, 0, speed2);
}

// Function to make the car turn left
void Car_Left(YB_Pcb_Car* car, int speed1, int speed2) {
    Ctrl_Car(car, 0, speed1, 1, speed2);
}

// Function to make the car turn right
void Car_Right(YB_Pcb_Car* car, int speed1, int speed2) {
    Ctrl_Car(car, 1, speed1, 0, speed2);
}

// Function to make the car spin left
void Car_Spin_Left(YB_Pcb_Car* car, int speed1, int speed2) {
    Ctrl_Car(car, 0, speed1, 1, speed2);
}

// Function to make the car spin right
void Car_Spin_Right(YB_Pcb_Car* car, int speed1, int speed2) {
    Ctrl_Car(car, 1, speed1, 0, speed2);
}

// Function to control the servo motor
void Ctrl_Servo(YB_Pcb_Car* car, int id, int angle) {
    if (angle < 0) {
        angle = 0;
    }
    else if (angle > 180) {
        angle = 180;
    }
    uint8_t data[2] = { (uint8_t)id, (uint8_t)angle };
    write_array(car, 0x03, data, 2);
}


#define TRACKING_RIGHT1 0 // WiringPi pin 11, BCM_GPIO 17
#define TRACKING_RIGHT2 7 // WiringPi pin 7, BCM_GPIO 27
#define TRACKING_LEFT1 2  // WiringPi pin 13, BCM_GPIO 22
#define TRACKING_LEFT2 3  // WiringPi pin 15, BCM_GPIO 23

void setup() {
    wiringPiSetup();
    pinMode(TRACKING_RIGHT1, INPUT);
    pinMode(TRACKING_RIGHT2, INPUT);
    pinMode(TRACKING_LEFT1, INPUT);
    pinMode(TRACKING_LEFT2, INPUT);
}
/*straight1 : 1111 >> go straight
straight2 : 1111 >> call left
straight3 : 1111 >> call right
left : 0110 >> call straight1
right : 0110 >> call straight1

1st >> means QR receive
2st >> intersection
3st >> done turn

case1) QR >> STRAIGHT == straight1 >> straight1

case2) QR >> LEFT == straight1 >> straight2 >> left >> straight1

case3) QR >> RIGHT == straight1 >> straight3 >> right >> straight1

case4) QR >> BACK == straight1 >> left >> straight1*/
void left_car();
void right_car();


void straight1(YB_Pcb_Car* car){
    int Left1 = digitalRead(TRACKING_LEFT1);
    int Left2 = digitalRead(TRACKING_LEFT2);
    int Right1 = digitalRead(TRACKING_RIGHT1);
    int Right2 = digitalRead(TRACKING_RIGHT2);

    // 0000 No detect
    if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Right(car, 40, 40);
        //printf("0000\n");
        delay(60);
    }
    // 0001 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0001\n");
        delay(60);
    }
    // 0011   Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0011\n");
        delay(60);
    }
    // 0110 Run
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Run(car, 55, 55);
        //printf("0110\n");
        delay(60);
    }
    // 0111 Run
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        Car_Run(car, 55, 55);
        //printf("0111\n");
        delay(60);
    }
    // 1000 Left
    else if (Left1 == LOW && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1000\n");
        delay(60);
    }
    // 1100 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1100\n");
        delay(60);
    }
    // 1110 Run
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Run(car, 55, 55);
        //printf("1110\n");
        delay(60);
    }
    // 1111 Stop
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        Car_Run(car, 55, 55);
        //printf("1111\n");
        delay(60);
    }
    // 0010 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Right(car, 50,50);
        delay(40);
    } 
    // 0100 LEFT
    else if (Left1 == HIGH && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50,50);
        delay(40);
    }
}



///////////LEFT////////////
void straight2(YB_Pcb_Car* car){
    int Left1 = digitalRead(TRACKING_LEFT1);
    int Left2 = digitalRead(TRACKING_LEFT2);
    int Right1 = digitalRead(TRACKING_RIGHT1);
    int Right2 = digitalRead(TRACKING_RIGHT2);

    // 0000 No detect
    if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 40, 40);
        //printf("0000\n");
        delay(60);
    }
    // 0001 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0001\n");
        delay(60);
    }
    // 0011   Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0011\n");
        delay(60);
    }
    // 0110 Run
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Run(car, 55, 55);
        //printf("0110\n");
        delay(60);
    }
    // 0111 Right
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0111\n");
        delay(60);
    }
    // 1000 Left
    else if (Left1 == LOW && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1000\n");
        delay(60);
    }
    // 1100 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1100\n");
        delay(60);
    }
    // 1110 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        printf("linetracer: 1110 LEFT\n");
        Car_Stop(car);
        delay(500);
        Car_Run(car,40,40);
        delay(500);
        Car_Spin_Left(car,60,60);
        delay(800);
        while(1){
            left_car(car);
            int Left1 = digitalRead(TRACKING_LEFT1);
            int Left2 = digitalRead(TRACKING_LEFT2);
            int Right1 = digitalRead(TRACKING_RIGHT1);
            int Right2 = digitalRead(TRACKING_RIGHT2);
            if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH){
                Car_Stop(car);
                delay(500);
                break;
            }  
        }
    }
    // 1111 Stop
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        printf("linetracer: 1111 LEFT\n");
        Car_Stop(car);
        delay(500);
        Car_Run(car,40,40);
        delay(500);
        Car_Spin_Left(car,60,60);
        delay(800);
        
        while(1){
            left_car(car);
            int Left1 = digitalRead(TRACKING_LEFT1);
            int Left2 = digitalRead(TRACKING_LEFT2);
            int Right1 = digitalRead(TRACKING_RIGHT1);
            int Right2 = digitalRead(TRACKING_RIGHT2);
            if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH){
                Car_Stop(car);
                delay(500);
                break;
            }  
        }
    }
        // 0010 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Right(car, 50,50);
        delay(40);
    } 
    // 0100 LEFT
    else if (Left1 == HIGH && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50,50);
        delay(40);
    }
}

///////////RIGHT////////////
void straight3(YB_Pcb_Car* car){
    int Left1 = digitalRead(TRACKING_LEFT1);
    int Left2 = digitalRead(TRACKING_LEFT2);
    int Right1 = digitalRead(TRACKING_RIGHT1);
    int Right2 = digitalRead(TRACKING_RIGHT2);

    // 0000 No detect
    if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Right(car, 40, 40);
        //printf("0000\n");
        delay(60);
    }
    // 0001 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0001\n");
        delay(60);
    }
    // 0011   Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0011\n");
        delay(60);
    }
    // 0110 Run
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Run(car, 55, 55);
        //printf("0110\n");
        delay(60);
    }
    // 0111 Right
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        printf("linetracer: 0111 RIGHT\n");
        Car_Stop(car);
        delay(500);
        Car_Run(car,40,40);
        delay(500);
        Car_Spin_Right(car,60,60);
        delay(800);
        while(1){
            right_car(car);
            int Left1 = digitalRead(TRACKING_LEFT1);
            int Left2 = digitalRead(TRACKING_LEFT2);
            int Right1 = digitalRead(TRACKING_RIGHT1);
            int Right2 = digitalRead(TRACKING_RIGHT2);
            if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
                Car_Stop(car);
                delay(500);
                break;
            }
        }
    }
    // 1000 Left
    else if (Left1 == LOW && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1000\n");
        delay(60);
    }
    // 1100 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1100\n");
        delay(60);
    }
    // 1110 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1110\n");
        delay(60);
    }
    // 1111 Stop
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        printf("linetracer: 1111 RIGHT\n");
        Car_Stop(car);
        delay(500);
        Car_Run(car,40,40);
        delay(500);
        Car_Spin_Right(car,60,60);
        delay(800);
        while(1){
            right_car(car);
            int Left1 = digitalRead(TRACKING_LEFT1);
            int Left2 = digitalRead(TRACKING_LEFT2);
            int Right1 = digitalRead(TRACKING_RIGHT1);
            int Right2 = digitalRead(TRACKING_RIGHT2);
            if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
                Car_Stop(car);
                delay(500);
                break;
            }
        }
    }
        // 0010 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Right(car, 50,50);
        delay(40);
    } 
    // 0100 LEFT
    else if (Left1 == HIGH && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50,50);
        delay(40);
    }
}
void left_car(YB_Pcb_Car* car){
    int Left1 = digitalRead(TRACKING_LEFT1);
    int Left2 = digitalRead(TRACKING_LEFT2);
    int Right1 = digitalRead(TRACKING_RIGHT1);
    int Right2 = digitalRead(TRACKING_RIGHT2);
    // 0000 No detect
    if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 60, 60);
        //printf("0000\n");
        delay(60);
    }
    // 0001 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0001\n");
        delay(60);
    }
    // 0011   Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0011\n");
        delay(60);
    }
    // 0110 Run
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {

        //while(1){straight1(car);}
        //printf("0110\n");
        delay(60);
    }
    // 0111 Right
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0111\n");
        delay(60);
    }
    // 1000 Left
    else if (Left1 == LOW && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1000\n");
        delay(60);
    }
    // 1100 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1100\n");
        delay(60);
    }
    // 1110 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1110\n");
        delay(60);
    }
    // 1111 Stop
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        //printf("1111\n");
    }
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Right(car, 50,50);
        delay(40);
    } 
    // 0100 LEFT
    else if (Left1 == HIGH && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50,50);
        delay(40);
    }
}

void right_car(YB_Pcb_Car* car){
    int Left1 = digitalRead(TRACKING_LEFT1);
    int Left2 = digitalRead(TRACKING_LEFT2);
    int Right1 = digitalRead(TRACKING_RIGHT1);
    int Right2 = digitalRead(TRACKING_RIGHT2);
    // 0000 No detect
    if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Right(car, 60, 60);
        //printf("0000\n");
        delay(60);
    }
    // 0001 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0001\n");
        delay(60);
    }
    // 0011   Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0011\n");
        delay(60);
    }
    // 0110 Run
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        delay(60);
    }
    // 0111 Right
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0111\n");
        delay(60);
    }
    // 1000 Left
    else if (Left1 == LOW && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1000\n");
        delay(60);
    }
    // 1100 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1100\n");
        delay(60);
    }
    // 1110 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1110\n");
        delay(60);
    }
    // 1111 Stop
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        //printf("1111\n");
    }
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Right(car, 50,50);
        delay(40);
    } 
    // 0100 LEFT
    else if (Left1 == HIGH && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50,50);
        delay(40);
    }
}

void drive(YB_Pcb_Car* car, const char* state){
    printf("Drive Called :");
    if (strcmp(state, "STRAIGHT") == 0){
        printf("STRAIGHT\n");
        while(1){straight1(car);}
    }
    else if (strcmp(state, "LEFT") == 0){
        printf("LEFT\n");
        while(1){straight2(car);}
    }
    else if (strcmp(state, "RIGHT") == 0){
        printf("RIGHT\n");
        while(1){straight3(car);}
    } 
    else if (strcmp(state, "BACK") == 0){
        Car_Stop(car);
        delay(500);
        Car_Spin_Left(car,60,60);
        delay(300);
        while(1){left_car(car);}
    }
}


///////////RIGHT////////////
void straight4(YB_Pcb_Car* car){
    int Left1 = digitalRead(TRACKING_LEFT1);
    int Left2 = digitalRead(TRACKING_LEFT2);
    int Right1 = digitalRead(TRACKING_RIGHT1);
    int Right2 = digitalRead(TRACKING_RIGHT2);

    // 0000 No detect
    if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Right(car, 40, 40);
        //printf("0000\n");
        delay(60);
    }
    // 0001 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0001\n");
        delay(60);
    }
    // 0011   Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        //printf("0011\n");
        delay(60);
    }
    // 0110 Run
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Run(car, 55, 55);
        //printf("0110\n");
        delay(60);
    }
    // 0111 Right
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        printf("linetracer: 0111 RIGHT\n");
        Car_Stop(car);
        delay(500);
        Car_Run(car,40,40);
        delay(500);
        Car_Spin_Right(car,60,60);
        delay(2300);
        while(1){
            right_car(car);
            int Left1 = digitalRead(TRACKING_LEFT1);
            int Left2 = digitalRead(TRACKING_LEFT2);
            int Right1 = digitalRead(TRACKING_RIGHT1);
            int Right2 = digitalRead(TRACKING_RIGHT2);
            if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
                Car_Stop(car);
                delay(500);
                break;
            }
        }
    }
    // 1000 Left
    else if (Left1 == LOW && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1000\n");
        delay(60);
    }
    // 1100 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        //printf("1100\n");
        delay(60);
    }
    // 1110 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        printf("linetracer: 0111 RIGHT\n");
        Car_Stop(car);
        delay(500);
        Car_Run(car,55, 55);
        delay(500);
        Car_Spin_Left(car,60,60);
        delay(2300);
        while(1){
            left_car(car);
            int Left1 = digitalRead(TRACKING_LEFT1);
            int Left2 = digitalRead(TRACKING_LEFT2);
            int Right1 = digitalRead(TRACKING_RIGHT1);
            int Right2 = digitalRead(TRACKING_RIGHT2);
            if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
                Car_Stop(car);
                delay(500);
                break;
            }
        }
    }
    // 1111 Stop
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        printf("linetracer: 1111 RIGHT\n");
        Car_Stop(car);
        delay(500);
        Car_Run(car,40,40);
        delay(500);
        Car_Spin_Right(car,60,60);
        delay(2300);
        while(1){
            right_car(car);
            int Left1 = digitalRead(TRACKING_LEFT1);
            int Left2 = digitalRead(TRACKING_LEFT2);
            int Right1 = digitalRead(TRACKING_RIGHT1);
            int Right2 = digitalRead(TRACKING_RIGHT2);
            if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
                Car_Stop(car);
                delay(500);
                break;
            }
        }
    }
        // 0010 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Right(car, 50,50);
        delay(40);
    } 
    // 0100 LEFT
    else if (Left1 == HIGH && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50,50);
        delay(40);
    }
}
void test(YB_Pcb_Car* car){
        while(1){straight4(car);}
    }

void* updateCommand(void* arg) {
    while (1) {
        FILE *file = fopen("direction_log.txt", "r");
        if (file == NULL) {
            perror("Failed to open direction_log.txt");
            usleep(100000);
            continue;
        }

        char newCommand[10];
        if (fgets(newCommand, sizeof(newCommand), file) != NULL) {
            // Remove newline character
            newCommand[strcspn(newCommand, "\n")] = '\0';
            if (strcmp(currentCommand, newCommand) != 0) {
                strcpy(currentCommand, newCommand);
                printf("Updated command: %s\n", currentCommand);
            }
        }

        fclose(file);
        usleep(200000);
    }
    return NULL;
}
void executeCommand(YB_Pcb_Car* car) {
    if (strcmp(currentCommand, "STRAIGHT") == 0) {
        printf("straight command on\n");
        straight1(car);
    }
    else if (strcmp(currentCommand, "LEFT") == 0) {
        printf("left command on\n");
        straight2(car);
        printf("left command done\n");
    }
    else if (strcmp(currentCommand, "RIGHT") == 0) {
        printf("right command on\n");
        straight3(car);
        printf("right command done\n");
    }
    else if (strcmp(currentCommand, "BACK") == 0) {
        printf("back command on\n");
        straight4(car);
        printf("back command done\n");
    }
}

int main() {
    YB_Pcb_Car car;
    car.addr = 0x16;
    if (get_i2c_device(&car, car.addr, 1) < 0) {
        return 1;
    }

    setup();
    Car_Stop(&car);
    delay(1000);

    pthread_t updateThread;
    pthread_create(&updateThread, NULL, updateCommand, NULL);

    while (1) {
        //straight2(&car);
        //Car_Stop(&car);
        
        executeCommand(&car);
        usleep(50000);
    }

    pthread_join(updateThread, NULL);
    return 0;
}