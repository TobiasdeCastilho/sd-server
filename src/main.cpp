#include <stdio.h>
#include <iostream>
#include <string.h>

#include "out.cpp"
#include "server.cpp"
#include "sdtp.cpp"

SDTP::Server stdp_server;		

int main(int argc, char** argv){  	
	if(argc < 2) {
		_ERROR << "Usage: " << argv[0] << " <port>" << std::endl;
		exit(1);
	}

	// Create a socket
	const int _port = atoi(argv[1]);	
	Server server(_port, TCP);		

	// Set the server function
	server.set_server_function([](int connection) -> void {
		stdp_server.recieve_request(connection);
	});		

	// Start listening
	server.listen_start(10);

	// Wait for the server to stop
	getchar();
	server.listen_stop();

	return 0;
}
