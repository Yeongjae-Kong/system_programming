# Makefile

# 컴파일러와 플래그 정의
CC = gcc
CXX = g++
CFLAGS = -Wall -g
LDFLAGS = -lpthread
WIRINGPI_FLAGS = -lwiringPi
OPENCV_FLAGS = `pkg-config --cflags --libs opencv4`

# 타겟 정의
TARGETS = stop server qrrec algorithm linetracer

# 디폴트 타겟
all: $(TARGETS)

# 각 타겟의 빌드 규칙
stop: stop.c
	$(CC) $(CFLAGS) stop.c -o stop $(WIRINGPI_FLAGS) $(LDFLAGS) -lm

server: server.c
	$(CC) $(CFLAGS) server.c -o server $(LDFLAGS)

qrrec: qrrec.cpp
	$(CXX) qrrec.cpp -o qrrec $(OPENCV_FLAGS)

algorithm: algorithm.c
	$(CC) algorithm.c -o algorithm -lm $(LDFLAGS)

linetracer: linetracer.c
	$(CC) $(CFLAGS) linetracer.c -o linetracer $(WIRINGPI_FLAGS) $(LDFLAGS)

# 클린업 규칙
clean:
	rm -f $(TARGETS)
	rm -f *.o
