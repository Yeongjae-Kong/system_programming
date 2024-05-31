#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <wiringPi.h>
#include <unistd.h> // sleep 함수를 위해 필요
#include <time.h>   // clock 함수를 위해 필요

// Define the YB_Pcb_Car struct
typedef struct {
    int file;
    int addr;
} YB_Pcb_Car;

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

void tracing_right(YB_Pcb_Car* car) {
    int Left1 = digitalRead(TRACKING_LEFT1);
    int Left2 = digitalRead(TRACKING_LEFT2);
    int Right1 = digitalRead(TRACKING_RIGHT1);
    int Right2 = digitalRead(TRACKING_RIGHT2);

    // 0000 No detect
    if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Right(car, 50, 50);
        printf("0000\n");
        delay(100);
    }
    // 0001 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        printf("0001\n");
        delay(100);
    }
    // 0011   Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 70, 70);
        printf("0011\n");
        delay(100);
    }
    // 0110 Run
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Run(car, 40, 40);
        printf("0110\n");
        delay(100);
    }
    // 0111 Right
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        printf("0111\n");
        delay(100);
    }
    // 1000 Left
    else if (Left1 == LOW && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        printf("1000\n");
        delay(100);
    }
    // 1100 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 70, 70);
        printf("1100\n");
        delay(100);
    }
    // 1110 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        printf("1110\n");
        delay(100);
    }
    // 1111 Stop
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        Car_Stop(car);
        printf("1111\n");
        delay(300);
        // Car_Spin_Right(car, 50, 30);
        // delay(1000);

    }
}

void tracing_left(YB_Pcb_Car* car) {
    int Left1 = digitalRead(TRACKING_LEFT1);
    int Left2 = digitalRead(TRACKING_LEFT2);
    int Right1 = digitalRead(TRACKING_RIGHT1);
    int Right2 = digitalRead(TRACKING_RIGHT2);

    // 0000 No detect
    if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        printf("0000\n");
        delay(100);
    }
    // 0001 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        printf("0001\n");
        delay(100);
    }
    // 0011   Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 70, 70);
        printf("0011\n");
        delay(100);
    }
    // 0110 Run
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Run(car, 40, 40);
        printf("0110\n");
        delay(100);
    }
    // 0111 Right
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        printf("0111\n");
        delay(100);
    }
    // 1000 Left
    else if (Left1 == LOW && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 70, 70);
        printf("1000\n");
        delay(100);
    }
    // 1100 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 70, 70);
        printf("1100\n");
        delay(100);
    }
    // 1110 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        printf("1110\n");
        delay(100);
    }
    // 1111 Stop
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        // Car_Stop(car);
        // delay(300);
        printf("1111\n");
        // Car_Spin_Left(car, 30, 50);
        // delay(1000);
    }
}
void tracing_back(YB_Pcb_Car* car) {
    int Left1 = digitalRead(TRACKING_LEFT1);
    int Left2 = digitalRead(TRACKING_LEFT2);
    int Right1 = digitalRead(TRACKING_RIGHT1);
    int Right2 = digitalRead(TRACKING_RIGHT2);
    Car_Spin_Left(car, 50, 50);
    delay(1000);
    // 0000 No detect
    if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        printf("0000\n");
        delay(100);
    }
    // 0001 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        printf("0001\n");
        delay(100);
    }
    // 0011   Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 70, 70);
        printf("0011\n");
        delay(100);
    }
    // 0110 Run
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Run(car, 40, 40);
        printf("0110\n");
        delay(100);
    }
    // 0111 Right
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        printf("0111\n");
        delay(100);
    }
    // 1000 Left
    else if (Left1 == LOW && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 70, 70);
        printf("1000\n");
        delay(100);
    }
    // 1100 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 70, 70);
        printf("1100\n");
        delay(100);
    }
    // 1110 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        printf("1110\n");
        delay(100);
    }
    // 1111 Stop
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        // Car_Stop(car);
        // delay(300);
        printf("1111\n");
        // Car_Spin_Left(car, 30, 50);
        // delay(1000);
    }
}


void tracing_straight(YB_Pcb_Car* car) {
    int Left1 = digitalRead(TRACKING_LEFT1);
    int Left2 = digitalRead(TRACKING_LEFT2);
    int Right1 = digitalRead(TRACKING_RIGHT1);
    int Right2 = digitalRead(TRACKING_RIGHT2);

    // 0000 No detect
    if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Run(car, 120, 120);
        printf("0000\n");
        delay(100);
    }
    // 0001 Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == HIGH && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        printf("0001\n");
        delay(100);
    }
    // 0011   Right
    else if (Left1 == HIGH && Left2 == HIGH && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        printf("0011\n");
        delay(100);
    }
    // 0110 Run
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Run(car, 150, 150);
        printf("0110\n");
        delay(100);
    }
    // 0111 Right
    else if (Left1 == HIGH && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        Car_Spin_Right(car, 50, 50);
        printf("0111\n");
        delay(100);
    }
    // 1000 Left
    else if (Left1 == LOW && Left2 == HIGH && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        printf("1000\n");
        delay(100);
    }
    // 1100 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == HIGH && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        printf("1100\n");
        delay(100);
    }
    // 1110 Left
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == HIGH) {
        Car_Spin_Left(car, 50, 50);
        printf("1110\n");
        delay(100);
    }
    // 1111 Stop
    else if (Left1 == LOW && Left2 == LOW && Right1 == LOW && Right2 == LOW) {
        // Car_Run(car, 40, 40);
        printf("1111\n");
        delay(100);
    }
}


int main() {

    YB_Pcb_Car car;
    car.addr = 0x16;
    if (get_i2c_device(&car, car.addr, 1) < 0) {
        return 1;
    }

    setup();

    clock_t start_time = clock();

    // 3초 동안 tracing_straight를 반복적으로 호출
    while ((clock() - start_time) < 3 * CLOCKS_PER_SEC) {
        tracing_straight(&car);
    }

    // 3초 후에 tracing_back 호출
    while(1){tracing_back(&car);}

    // Car_Stop(&car);

    return 0;
}
