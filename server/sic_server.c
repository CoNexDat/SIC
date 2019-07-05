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
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <sys/shm.h>
#include <openssl/ecdsa.h>   // for ECDSA_do_sign, ECDSA_do_verify
#include "../server/ntp-packet.h"
#include "aux.h"

#define PKEYFILE "serverPrivKey.pem"
#define PBKEYFILE "serverPubKey.pem"
#define CLNTPUBKEYFILE "clientPubKey.pem"

EC_KEY *load_key(char *PRIVKEYFILE,char *PUBKEYFILE);
EC_KEY *load_pub_key(char *PUBKEYFILE);
EC_KEY *eckey; // server private key
EC_KEY *clntkey; // client public key

int start_values()
{
	int select_option;
	char aux_line[50];
	size_t len;
	char *token;
	FILE* finput;	
	
	// File path //
	getcwd(file_path, sizeof(file_path));
	strncat(file_path,"/",sizeof(file_path));
	// Read configuration file //
	strncpy(config,"config.conf",sizeof(config));
	strncpy(file_config,file_path,sizeof(file_config));
	strncat(file_config,config,sizeof(file_config));
	finput = fopen(file_config, "rt");
    select_option=0;
    while(!feof(finput))
    {
        fgets(aux_line, sizeof(aux_line), finput);
        token = strtok(aux_line, ":");
        token=strtok(NULL,"\0");
		switch (select_option) 
		{
			case 0:
				src_port=atof(token);
				select_option++;
				continue;				
			case 1:
				strncpy(src_ip,token,sizeof(src_ip));
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
	
    strftime(output, sizeof(output), "%d-%m-%y %H:%M:%S--", tlocal);
    strncat(output,msg,sizeof(output));
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
	signed_ntp_packet packet,packet_old;
	memset( &packet, 0, sizeof( ntp_packet ) );
	memset( &packet_old, 0, sizeof( ntp_packet ) );

	addr_size = sizeof serverStorage;
	while(1){
		nBytes = recvfrom(udpSocket,&packet,sizeof(packet),0,(struct sockaddr *)&serverStorage, &addr_size);
		// Verify client signature
		ECDSA_SIG sig;
		BIGNUM* r = BN_new();
		BIGNUM* s = BN_new();
		BN_bin2bn(packet.signature_r,sizeof(packet.signature_r),r);
		BN_bin2bn(packet.signature_s,sizeof(packet.signature_s),s);
		sig.s=s;
		sig.r=r;
		if (1 != ECDSA_do_verify((char *)(&packet.ntp), sizeof(packet.ntp), &sig, clntkey)) {
		        printf("Failed to verify Client EC Signature\n");
			continue;
		} else {
		        printf("Verified Client EC Signature\n");
			}

		t2=get_timestamp();
		packet.ntp.T2Tm_s = t2/1000000ll;
		packet.ntp.T2Tm_f = t2-packet.ntp.T2Tm_s;
		t3=get_timestamp();
		packet.ntp.T3Tm_s = t3/1000000ll;
		packet.ntp.T3Tm_f = t3-packet.ntp.T3Tm_s;
		// Sign the last packet message
                ECDSA_SIG *signature = ECDSA_do_sign((char *)(&packet_old.ntp), sizeof(packet_old.ntp), eckey);
		memcpy(&packet_old.ntp,&packet.ntp,sizeof(packet_old.ntp));
		BN_bn2bin(signature->r,packet.signature_r);
		BN_bn2bin(signature->s,packet.signature_s);
		sendto(udpSocket,&packet,sizeof(packet),0,(struct sockaddr *)&serverStorage,addr_size);
	}

}


void main()
{
	eckey = load_key(PKEYFILE,PBKEYFILE);
	clntkey = load_pub_key(CLNTPUBKEYFILE);
	start_values();
	create_socket();
	wait_connections();	
}
