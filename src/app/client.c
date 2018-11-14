#include <stdio.h>
#include <stdlib.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <string.h> 
#include "telemetry.h"

#define UNUSED(x) ((void)(x))

int main(int argc, char **argv)
{
	puts("Running Client: ");
	UNUSED(argc);
	UNUSED(argv);
	channel_manifest_t *telem_manifest;

	//Create Channels for the incoming telem so manifest is ready to send 
	telem_manifest = channel_manifest_create(TELEMETRY_CAPACITY);

	//ACCEL. CHANNELS 
	channel_add(telem_manifest,"accel_x", "m/s^2",TELEM_UINT16,sizeof(uint16_t));
	channel_add(telem_manifest,"accel_y", "m/s^2",TELEM_UINT16,sizeof(uint16_t));
	channel_add(telem_manifest,"accel_z", "m/s^2",TELEM_UINT16,sizeof(uint16_t));	

	//GYRO. CHANNELS 
	channel_add(telem_manifest,"gyro_x", "deg/s",TELEM_UINT16,sizeof(uint16_t));
	channel_add(telem_manifest,"gyro_y", "deg/s",TELEM_UINT16,sizeof(uint16_t));
	channel_add(telem_manifest,"gyro_z", "deg/s",TELEM_UINT16,sizeof(uint16_t));

	//channel_print(stdout, &telem_manifest->channels[1]);
	channel_manifest_print(stdout,telem_manifest);
	
	//CREATE A SOCKET 
	int network_socket; 
	network_socket = socket(AF_INET, SOCK_STREAM, 0);

	//SPECIFY AN ADDRESS FOR THE SOCKET 
	struct sockaddr_in server_address; 
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9002);
	server_address.sin_addr.s_addr = INADDR_ANY;

	int connection_status = connect(network_socket,(struct sockaddr *) &server_address,sizeof(server_address));
	if(connection_status == -1){
		printf("Error making connection to socket");
	}

	//receive data from server
	char server_response[256];
	recv(network_socket,&server_response,sizeof(server_response),0);

	//print out server's response 
	printf("Server send: %s\n", server_response);

	close(network_socket);
	
	//CHANNELS AND MANIFEST CREATED, NEED TO SEND TO SERVER 

	//NEED TO SEND PACKETS TO SERVER 

	return 0;
}
