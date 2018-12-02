#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "telemetry.h"

#define UNUSED(x) ((void)(x))

int main(int argc, char **argv)
{
	static const char ip_addr[] = "172.16.42.157";

	puts("Running Client: ");
	UNUSED(argc);
	UNUSED(argv);
	channel_manifest_t *telem_manifest;

	//Create Channels for the incoming telem so manifest is ready to send 
	telem_manifest = channel_manifest_create(TELEMETRY_CAPACITY);

	//ACCEL. CHANNELS 
	channel_add(telem_manifest,"accel_x", "m/s^2",TELEM_UINT32,sizeof(uint32_t));
	channel_add(telem_manifest,"accel_y", "m/s^2",TELEM_UINT32,sizeof(uint32_t));
	channel_add(telem_manifest,"accel_z", "m/s^2",TELEM_UINT32,sizeof(uint32_t));	

	//GYRO. CHANNELS 
	channel_add(telem_manifest,"gyro_x", "deg/s",TELEM_UINT32,sizeof(uint32_t));
	channel_add(telem_manifest,"gyro_y", "deg/s",TELEM_UINT32,sizeof(uint32_t));
	channel_add(telem_manifest,"gyro_z", "deg/s",TELEM_UINT32,sizeof(uint32_t));

	//PID. CHANNELS 
	channel_add(telem_manifest,"pid_pitch", "deg/s",TELEM_UINT32,sizeof(uint32_t));
	channel_add(telem_manifest,"pid_roll", "deg/s",TELEM_UINT32,sizeof(uint32_t));
	channel_add(telem_manifest,"pid_yaw", "deg/s",TELEM_UINT32,sizeof(uint32_t));

	//ESC CHANNELS 
	channel_add(telem_manifest,"esc_front_left", "percent",TELEM_UINT32,sizeof(uint32_t));
	channel_add(telem_manifest,"esc_front_right", "percent",TELEM_UINT32,sizeof(uint32_t));
	channel_add(telem_manifest,"esc_back_left", "percent",TELEM_UINT32,sizeof(uint32_t));
	channel_add(telem_manifest,"esc_back_right", "percent",TELEM_UINT32,sizeof(uint32_t));

	//Battery CHANNELS 
	channel_add(telem_manifest,"batt_v_cell1", "voltage",TELEM_UINT32,sizeof(uint32_t));
	channel_add(telem_manifest,"batt_v_cell2", "voltage",TELEM_UINT32,sizeof(uint32_t));
	channel_add(telem_manifest,"batt_v_cell3", "voltage",TELEM_UINT32,sizeof(uint32_t));
	channel_add(telem_manifest,"batt_v_total", "voltage",TELEM_UINT32,sizeof(uint32_t));
	
	channel_add(telem_manifest,"batt_current", "amps",TELEM_UINT32,sizeof(uint32_t));


	//channel_print(stdout, &telem_manifest->channels[1]);
	//channel_manifest_print(stdout,telem_manifest);
	
	puts("Running Client: \n");
	UNUSED(argc);
	UNUSED(argv);

	//CREATE A SOCKET
	int network_socket;
	network_socket = socket(AF_INET, SOCK_STREAM, 0);

	//SPECIFY AN ADDRESS FOR THE SOCKET
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(5000);
	server_address.sin_addr.s_addr = inet_addr(ip_addr);

	int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	char char_buffer[256];
	
	//On connection, send over channel manifest if no connection error 	
	if(connection_status == -1){
		perror("Error making connection to socket\n");
   		return 1;
	}
	else {
		printf("Connected to server on port 5000\n");
		printf("Sending Manifest to server\n");
		//Send all manifest channel information byte by byte
		for (unsigned int i = 0; i < telem_manifest->count; i++) {
			channel_t *channel = &telem_manifest->channels[i];
			*((uint32_t *) char_buffer) = channel->manifest_index; // uint32_t, 4 bytes
			*((channel_data_t *) &char_buffer[sizeof(uint32_t)]) = channel->type; // channel_data_t (enum),4 bytes
			*((size_t *) &char_buffer[sizeof(uint32_t) + sizeof(channel_data_t)]) = channel->size; // size_t, should be 4 bytes
			sprintf(&char_buffer[sizeof(uint32_t) + sizeof(channel_data_t) + sizeof(size_t)], "%s,%s\n",
				channel->name, channel->unit);
			
			printf("------------Channel %d---------------\n",i);
			printf("Channel Index: %u (%u)\n", *((uint32_t *) char_buffer), channel->manifest_index);
			printf("Channel Type: %s\n", channel_type_to_str(*((channel_data_t *) &char_buffer[sizeof(uint32_t)])));
			printf("Channel Size: %d \n", *((size_t *) &char_buffer[sizeof(uint32_t) + sizeof(channel_data_t)]));
			printf("Channel Name/Unit: %s \n",(char *) &char_buffer[sizeof(uint32_t) + sizeof(channel_data_t) + sizeof(size_t)]);
			fflush(stdout);

			//Send the buffer
		 	send(network_socket, char_buffer, sizeof(char_buffer), 0);
	 	}
		printf("Finished sending manifest\n");
	}
	//Close TCP
	close(network_socket);

	//New TCP connection to streamline data 
	int data_socket;
	data_socket = socket(AF_INET, SOCK_STREAM, 0);

	//specify address for new TCP socket
	struct sockaddr_in data_address;
	data_address.sin_family = AF_INET;
	data_address.sin_port = htons(6000);
	data_address.sin_addr.s_addr = inet_addr(ip_addr);

	connection_status = connect(data_socket, (struct sockaddr *) &data_address, sizeof(data_address));
	
	//TESTING ACCEL PACKETS 
	//CREATE A TEST MANIFEST, CREATE TEST PACKETS FROM IT 
	channel_manifest_t *test_manifest;
	test_manifest = channel_manifest_create(TELEMETRY_CAPACITY);
	//ACCEL. CHANNELS 
	channel_add(test_manifest,"accel_x", "m/s^2",TELEM_UINT32,sizeof(uint32_t));
	channel_add(test_manifest,"accel_y", "m/s^2",TELEM_UINT32,sizeof(uint32_t));
	channel_add(test_manifest,"accel_z", "m/s^2",TELEM_UINT32,sizeof(uint32_t));	

	channel_add(test_manifest,"gyro_x", "deg/s",TELEM_UINT32,sizeof(uint32_t));
	channel_add(test_manifest,"gyro_y", "deg/s",TELEM_UINT32,sizeof(uint32_t));
	channel_add(test_manifest,"gyro_z", "deg/s",TELEM_UINT32,sizeof(uint32_t));

	telemetry_packet_t **packets;
    size_t npackets;
	/* create telemetry packets for these channels */
    packets = telemetry_packets_from_manifest(test_manifest, 500, &npackets);
    if (!packets) return 1;
    printf("\n%u packets created.\r\n", npackets);
    
	for (unsigned int i = 0; i < npackets; i++)
        telemetry_packet_print(stdout, packets[i]);

    /* set data values */
    for (unsigned int i = 0; i < test_manifest->count; i++)
    {
        *((uint32_t *) test_manifest->channels[i].data) = 11;
    }

    /* print manifest */
    for (unsigned int i = 0; i < 6; i++){
        channel_print(stdout, &test_manifest->channels[i]);
	}

	//On connection, send over incoming data 
	if(connection_status == -1){
		perror("Error making connection to socket\n");
   		return 1;
	}
	else 
	{
		printf("Connected to server on port 6000\n");
		printf("Sending incoming data to server\n");
		for(unsigned int i = 0; i < npackets;i++){
			printf("the packet size: %u\n",telemetry_packet_size(packets[i]));
			send(data_socket, packets[i], telemetry_packet_size(packets[i]), 0);	
		}
	}
	//end main
	return 0;
}
