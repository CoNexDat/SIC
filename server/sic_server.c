//
//   sic_server.c is part of SIC.
//
//   Copyright (C) 2018  your name
//
//   SIC is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   SIC is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
//

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h> 
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <sys/shm.h>
#include "aux.h"

int start_values()
{
	int select_option;
	char aux_line[50];
	size_t len;
	char *token;
	FILE* finput;	
	
	// File path //
	getcwd(file_path, sizeof(file_path));
	strcat(file_path,"/");
	// Read configuration file //
	strcpy(config,"config.conf");
	strcpy(file_config,file_path);
	strcat(file_config,config);	
	finput = fopen(file_config, "rt");
    select_option=0;
    while(!feof(finput))
    {
        fgets(aux_line, 50, finput);
        token = strtok(aux_line, ":");
        token=strtok(NULL,"\0");
		switch (select_option) 
		{
			case 0:
				src_port=atof(token);
				select_option++;
				continue;				
			case 1:
				len = strlen(token)-1;
				strncpy(src_ip,token,len);
				select_option++;
				continue;						
		}
	}

	return 0;
}


void error( char* msg )
{
    time_t tiempo = time(0);
    struct tm *tlocal = localtime(&tiempo);
    char output[128];
	
    strftime(output, 128, "%d-%m-%y %H:%M:%S--", tlocal);
    strcat(output,msg);
    perror( output );	
}

long long int get_timestamp () 
{
	struct timeval timer_usec; 
	
	long long int timestamp_usec; // timestamp in microseconds //
	if (!gettimeofday(&timer_usec, NULL)) 
	{
		timestamp_usec = ((long long int) timer_usec.tv_sec) * 1000000ll + (long long int) timer_usec.tv_usec;
	}
	else
	{
		timestamp_usec = -1;
	}
	return timestamp_usec;
}


int create_socket()
{
	
	// Configure settings in address struct //
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(src_port);
	serverAddr.sin_addr.s_addr = inet_addr(src_ip);
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
	addr_size = sizeof serverAddr;

	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if ( udpSocket < 0 )
	{
		error("Error: can't create socket");
		exit(1);

	}
	if (bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))< 0)
	{
		close(udpSocket);
		error( "Error: bad ip or port" );
		exit(1);
	}
	return 0;
}

void wait_connections()
{
	int nBytes, i;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	long long int t2;
	long long int t3;
	char t2_str[17];
    char t2_pps_str[17];
	char t3_str[17];
	char chain_tx[67];
	char chain_rx[67];	

	addr_size = sizeof serverStorage;
	while(1){
		nBytes = recvfrom(udpSocket,chain_rx,sizeof(chain_rx),0,(struct sockaddr *)&serverStorage, &addr_size);
		t2=get_timestamp();
		sprintf(t2_str, "%lld", t2);
		strcpy(chain_tx, strtok(chain_rx,"|"));
		strcat(chain_tx,"|");
		strcat(chain_tx,t2_str);
		strcat(chain_tx,"|");
		t3=get_timestamp();
		sprintf(t3_str, "%lld", t3);
		strcat(chain_tx,t3_str);
		strcat(chain_tx,"|");
		strcat(chain_tx,"0000000000000000");
		memset(chain_rx,'\0',strlen(chain_rx));
		printf("%s\n",chain_tx);
		sendto(udpSocket,chain_tx,sizeof(chain_tx),0,(struct sockaddr *)&serverStorage,addr_size);
	}

}


void main()
{
	start_values();
	create_socket();
	wait_connections();	
}
