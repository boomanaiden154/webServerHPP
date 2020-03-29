main: main.cpp server.hpp state.hpp login.hpp websocketServer.hpp
	g++ main.cpp -lpthread -lssl -lcrypto -std=c++17 -g -o main
