201911012 공영재
202011037 김민재
202011094 박현우
202211068 류경찬

# 2024_SystemProgramming_Server
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
