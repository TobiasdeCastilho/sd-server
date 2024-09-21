#include <stdio.h>
#include <iostream>
#include <pthread.h>

#include "out.cpp"
#include "server.cpp"

int main(int argc, char** argv){  

	if(argc < 2) {
		_ERROR << "Usage: " << argv[0] << " <port>" << std::endl;
		exit(1);
	}

	// Create a socket
	const int _port = atoi(argv[1]);	
	Server server(_port, TCP);

	// Set the server function
	server.set_server_function(
		[](int id) -> void {					
			_SUCESS << "Connection " << id << " established" << std::endl;
			sleep(5);
			_SUCESS << "Connection " << id << " closed" << std::endl;		
		}
	);

	// Start listening
	server.listen_start(10);

	// Wait for the server to stop
	getchar();
	server.listen_stop();

	return 0;
}
