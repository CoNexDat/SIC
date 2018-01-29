
//
//   sic_client.c is part of SIC.
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

#include <gsl/gsl_fit.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>
#include <stdio.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <netdb.h>
#include <inttypes.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "fixed_window_bib.c"
#include "aux.h"
#include <unistd.h>

typedef struct  
{ 
	int epoch; 
	long long int t1; 
	long long int t2; 
	long long int t3; 
	long long int t4; 
	bool to;
} sic_data;

// Function to control duplicate times //
int validate(char t1[20], char t2[20], char t3[20], char t4[20])
{
	double t1_c, t2_c, t3_c, t4_c;
	int i;
	char t1_[11];
	char t2_[11];
	char t3_[11];
	char t4_[11];

	// Trim to get seconds part of timestamps //
	for(i=0;i<10;i++)  t1_[i]=t1[i]; t1_[10]= '\0';
		
	for(i=0;i<10;i++)  t2_[i]=t2[i]; t2_[10]= '\0';
	
	for(i=0;i<10;i++)  t3_[i]=t3[i]; t3_[10]= '\0';
	
	for(i=0;i<10;i++)  t4_[i]=t4[i]; t4_[10]= '\0';		
	
	// Convert to numeric format //
	t1_c = atof(t1_);
	
	if(t1_c <= 0)
		return 1;

	t2_c = atof(t2_); 
	
	if(t2_c <= 0)
		return 1;
	
	t3_c = atof(t3_);
	
	if(t3_c <= 0)
		return 1;	

	t4_c = atof(t4_);

	if(t4_c <= 0)
		return 1;	
	
	// Set the first time //
	if (t1_p==0 && t2_p==0 && t3_p==0 && t4_p==0) 
	{
		t1_p=t1_c;
		t2_p=t2_c;
		t3_p=t3_c;
		t4_p=t4_c;
		return 0;
	}
	else
	{	
		// Compare the current to previous seconds //
		if((t1_c > t1_p) && (t2_c > t2_p) && (t3_c > t3_p) && (t4_c > t4_p))
		{
			t1_p=t1_c;
			t2_p=t2_c;
			t3_p=t3_c;
			t4_p=t4_c;
			return 0;
		}
		else
			return 1;
	}
}

// Function to get median  //
double get_median(double *x,int size) 
{
    double median_;
    int i;
    double *a;
    
    a=malloc(sizeof(double)*size);
    for(i=0; i<size; i++) 
		a[i]=x[i];		
	gsl_sort(a,1,size);
	median_ = gsl_stats_median_from_sorted_data (a,1,size);
	free(a);
	
	return median_;
}



// Get system timestamp //
long long int get_timestamp () 
{
	struct timeval timer_usec; 
	// timestamp in microseconds //
	long long int timestamp_usec; 

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

void save_syn_values(char m_[20], char c_[20], char s_[10])
{   
	int  ch;
	char m_v [50];
	char c_v [50];
	char f_s [50];
	FILE *file;

    file = fopen(sync_values_path, "w+");
    if(file == NULL)
    {
        printf("Error: can't open file");
    }

    fprintf (file, "%s\t%s\t%s\n",m_,c_,s_);
    fclose(file);
    
    return;    
}


// Print errors //
void error( char* msg )
{
	time_t tiempo = time(0);
    struct tm *tlocal = localtime(&tiempo);
    char output[128];
	
    strftime(output, 128, "%d-%m-%y %H:%M:%S -- ", tlocal);
    strcat(output,msg);
    perror( output );
}


int create_socket()
{
    // Set client parameters //
	memset(&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;

	local_addr.sin_port = htons(CLIENT_PORT);
	local_addr.sin_addr.s_addr = inet_addr(CLIENT_IP);
	
	// Configure settings in address struct //
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
	
	addr_size = sizeof serverAddr;

	clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if ( clientSocket < 0 )
	{
		error( "Error: can't create socket" );
		exit(1);
	}
	if (bind(clientSocket, (struct sockaddr *)&local_addr, sizeof(local_addr))< 0)
	{
		close(clientSocket);
		error( "Error: bad ip or port" );
		exit(1);
	}
	flag_parameters = 1;

	return 0;
}


int start_values()
{
	char aux_line[50];
	int select_option;
	size_t len;
	char *token;
	FILE* finput;

	// File path //
	getcwd(file_path, sizeof(file_path));
	strcat(file_path,"/");
	strcpy(config,"config.conf");
	strcpy(file_config,file_path);
	strcat(file_config,config);	
	// Read configuration file //
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
				CLIENT_PORT=atof(token);
				select_option++;
				continue;	
			case 1:
				SERVER_PORT=atof(token);
				select_option++;				
				continue;				
			case 2:
				len = strlen(token)-1;
				strncpy(CLIENT_IP,token,len);
				select_option++;
				continue;				
			case 3:
				len = strlen(token)-1;
				strncpy(SERVER_IP,token,len);
				select_option++;
				continue;				
			case 4:
				MEDIAN_MAX_SIZE = atof(token);
				select_option++;
				continue;				
			case 5:
				P = atof(token);
				select_option++;
				continue;				
			case 6:
				alpha = atof(token);
				select_option++;
				continue;				
			case 7:
				err_RTT = atof(token);
				select_option++;
				continue;				
			case 8:
				TIMEOUT = atof(token);
				select_option++;			
		}
	}
	
	pre_sync=INT_MAX-P;
	epoch_sync=INT_MAX-P;
	strcat(sync_values_txt,"sync_values.dat");	
	strcpy(sync_values_path,file_path);
	strcat(sync_values_path,sync_values_txt);
	save_syn_values("0.0","0.0","NOSYNC");

	return 0;
}

// Send and Receive data server //
sic_data send_sic_packet(void)
{

	char chain_tx[67];
	char chain_rx[67];
	char tx_str[52]= "|0000000000000000|0000000000000000|0000000000000000";
	char t1_str[17];
	char t4_str[17];
	char t2_str_rec[17];
	char t3_str_rec[17];
	sic_data out;
	int flags, receive, i;
	long long int t1, t4;	
	int nBytes, validation, numfd;
	fd_set readfds;
	struct timeval tv;

	if (flag_parameters == 0)
		create_socket();	

	// Conect to server //
	if ( connect( clientSocket, ( struct sockaddr * ) &serverAddr, sizeof( serverAddr) ) < 0 ) 
	{
		error( "Error: connecting socket" ); 
		out.epoch=0;
		out.t1=0;
		out.t2=0;
		out.t3=0;
		out.t4=0;
		to=true;	
	}
	
	t1=get_timestamp();  
	sprintf(t1_str, "%lld", t1);	
	strcpy(chain_tx,t1_str);
	strcat(chain_tx,tx_str);	
	
	// Send data to server //
	if ( sendto(clientSocket,chain_tx,sizeof(chain_tx),0,(struct sockaddr *)&serverAddr,addr_size) < 0)
	{
		close(clientSocket);
		error( "Error: sentto");
		flag_parameters = 0;
		out.epoch=t1/1000000;
		out.t1=t1;
		out.t2=0;
		out.t3=0;
		out.t4=0;
		to=true;
	}	
	
	// Set socket no block //
	flags = fcntl(clientSocket, F_GETFL, 0);
	fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
	FD_ZERO(&readfds);
	FD_SET(clientSocket, &readfds);
	numfd = clientSocket + 1;	
	// Time to wait response //
	tv.tv_sec=0;
	tv.tv_usec=TIMEOUT;

	/* Receive server response */
	receive = select(numfd, &readfds,NULL,NULL,&tv);
    switch (receive) 
    {
		case -1:
			error("Error: data reception");
			FD_CLR(clientSocket, &readfds);
			close(clientSocket);
			flag_parameters = 0;
			out.epoch=t1/1000000;
			out.t1=t1;
			out.t2=0;
			out.t3=0;
			out.t4=0;
			to=true;
		case 0:
			error("Error: timeout");
			FD_CLR(clientSocket, &readfds);
			close(clientSocket);
			flag_parameters = 0;
			out.epoch=t1/1000000;
			out.t1=t1;
			out.t2=0;
			out.t3=0;
			out.t4=t4;
			to=true;
		default:
			// If exist data //
			if (FD_ISSET(clientSocket, &readfds)) 
			{
				// Get t4 //
				t4=get_timestamp();
				sprintf(t4_str, "%lld", t4);
				
				nBytes = recvfrom(clientSocket,chain_rx,sizeof(chain_rx),0,NULL, NULL);
				if (nBytes < 0)
				{
					error( "Error: recept data" );
					FD_CLR(clientSocket, &readfds);
					close(clientSocket);
					flag_parameters = 0;
					out.epoch= t1/1000000;
					out.t1=t1;
					out.t2=0;
					out.t3=0;
					out.t4=t4;
					to=true;				
				}
				// Clear fd variable //
				FD_CLR(clientSocket, &readfds);
				
				// Trim times //
				for(i=17;i<33;i++)  t2_str_rec[i-17]=chain_rx[i];
				t2_str_rec[16]= '\0';
								
				// t3 //
				for(i=34;i<51;i++)  t3_str_rec[i-34]=chain_rx[i];
				t3_str_rec[16]= '\0';		
				
				// validate duplicate times //
				validation=validate(t1_str, t2_str_rec, t3_str_rec, t4_str);
				
				if (validation <= 0)
				{
					out.epoch=t1/1000000;
					out.t1=t1;
					out.t2=atof(t2_str_rec);
					out.t3=atof(t3_str_rec);
					out.t4=t4;
					out.to=false;
				}
				else
				{
					printf("Invalid time %d\n",validation);
					out.epoch=t1/1000000;
					out.t1=t1;
					out.t2=0;
					out.t3=0;
					out.t4=t4;
					out.to=true;
				}
			}	
	}
	return out;
}


void process(sic_data sic_receive, fixed_window *W_m, fixed_window *W_median, fixed_window *W_RTT, fixed_window *W_epoch)
{
	double aux;
	double aux2;
	double a[P];
	double RTT_l;
	double RTT_f;	
	double m_RTT;
	double cov00, cov01, cov11, sumasq,m,c;
	char caux[20];
	char maux[20];
	
	printf(">> DEBUG Receive epoch %d - RTT %lf\n",sic_receive.epoch,(double)(sic_receive.t4 - sic_receive.t1));
	if(sic_receive.to==false)
	{
		// Estimate fi //
		aux=sic_receive.t1 - sic_receive.t2 +(sic_receive.t2 - sic_receive.t1 + sic_receive.t4 - sic_receive.t3)/2;
		fixed_window_insert(W_m,aux);	// Add fi into W_m window //
		fixed_window_insert(W_median,get_median(W_m->w,MEDIAN_MAX_SIZE));	// Get median and insert into W_median //
		fixed_window_insert(W_epoch,(double) sic_receive.epoch);		// Add epoch into W_epoch //
		fixed_window_insert(W_RTT,sic_receive.t4 - sic_receive.t1);		// Add rtt into W_RTT //
		fixed_window_Hread(W_RTT,0,a);
		RTT_f=gsl_stats_min(a, 1, P);
		fixed_window_Hread(W_RTT,1,a);
		RTT_l=gsl_stats_min(a, 1, P);
		m_RTT=gsl_stats_min(W_RTT->w,1,2*P);
		
		if(fabs(RTT_l-RTT_f)<=err_RTT*m_RTT) 
		{
			if(sic_receive.epoch >= (pre_sync + P))
			{
				printf("Sync = true\n");
				synck=true;
			}
		}
		else
		{
			printf(">> DEBUG Route change %lf >= %lf\n",fabs(RTT_l-RTT_f),err_RTT*m_RTT);
			save_syn_values("0","0", "NOSYNC");		
			synck=false;
			epoch_sync=INT_MAX-P;
			pre_sync=INT_MAX-P;
			err_sync = sic_receive.epoch;
			for(int i=0; i < MEDIAN_MAX_SIZE;i++ )
				W_m->w[i]=0;
			for(int i=0; i < P; i++)
				W_median->w[i]=0;
		}
		if((synck==true) && (sic_receive.epoch >= (epoch_sync + P)))
		{
			printf(">> DEBUG Linear fit \n");
			gsl_fit_linear(W_epoch->w, 1, W_median->w, 1, P, &c, &m, &cov00, &cov01, &cov11, &sumasq);
			actual_c=c;
			actual_m=(1-alpha)*m+(actual_m*alpha);
			epoch_sync = sic_receive.epoch;
			sprintf(maux,"%lf",actual_m);
			sprintf(caux,"%lf",actual_c);
			save_syn_values(maux,caux, "SYNC");
		}
		else
		{
			if(sic_receive.epoch >= (err_sync + MEDIAN_MAX_SIZE))
			{
				printf(">> DEBUG Complete W_m -> %d == %d\n",sic_receive.epoch,err_sync + MEDIAN_MAX_SIZE);
				pre_sync = sic_receive.epoch;
				err_sync = INT_MAX-MEDIAN_MAX_SIZE;
			}
			if(sic_receive.epoch >= pre_sync + P)
			{
				printf(">> DEBUG First linear fit (1 time)\n");
				gsl_fit_linear(W_epoch->w, 1, W_median->w, 1, P, &c, &m,&cov00, &cov01, &cov11, &sumasq);
				synck = true;
				epoch_sync = sic_receive.epoch;
				actual_m = m;
				actual_c = c;
				sprintf(maux,"%lf",actual_m);
				sprintf(caux,"%lf",actual_c);
				save_syn_values(maux,caux, "PRESYNC");				
			}
		}	
	}
	else
	{
		printf("Timeout\n");
		to=false;
	}
}

void main()
{
	int ret, limit;
	double min_rtt, rtt_tmp;
	sic_data receive;
	
	// Read config file //
	start_values();
	fflush(stdout);

	// Window for save medians //
	fixed_window *W_m = fixed_window_init(MEDIAN_MAX_SIZE); 		
	// Window for epoch //
	fixed_window *W_epoch = fixed_window_init(P);
	// Window for estimate median //
	fixed_window *W_median = fixed_window_init(P);
	// Window for RTT (route change) //
	fixed_window *W_RTT = fixed_window_init(P*2);
	
	// First packet to get first RTT to fill W_RTT
	receive=send_sic_packet();
	err_sync = receive.epoch;
	rtt_tmp = receive.t4 - receive.t1;
	
	for(int i = 0; i <= P*2; i++)
		fixed_window_insert(W_RTT,rtt_tmp);
	sleep(1);
	
	while(1)
	{
		receive=send_sic_packet();
		process(receive,W_m,W_median,W_RTT,W_epoch);
		sleep(1);	
	}
}
