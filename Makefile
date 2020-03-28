main: main.cpp server.hpp state.hpp login.hpp websocketServer.hpp
	g++ main.cpp -lpthread -std=c++17 -g -o main
