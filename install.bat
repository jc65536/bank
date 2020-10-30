curl -L "https://downloads.sourceforge.net/project/asio/asio/1.18.0%20%28Stable%29/asio-1.18.0.zip" --output asio-1.18.0.zip
powershell -command "Expand-Archive -Force asio-1.18.0.zip ."
del asio-1.18.0.zip
g++ src/server.cpp -o server.exe -I asio-1.18.0/include -l ws2_32 -l wsock32
g++ src/client.cpp -o client.exe -I asio-1.18.0/include -l ws2_32 -l wsock32