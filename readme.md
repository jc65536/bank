# Bank
Robotics team assignment
Requirements: [Asio](https://think-async.com/Asio/) (just the headers)
### How to compile (Windows)
Server
```
g++ src/server.cpp -o server.exe -I path/to/asio/include -l ws2_32 -l wsock32 -g
```
Client
```
g++ src/server.cpp -o server.exe -I path/to/asio/include -l ws2_32 -l wsock32 -g
```