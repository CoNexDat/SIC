##################################################################### 
#   Variables				  			    #
#####################################################################

OBJECTS = sic_client.o 
BINARY = -o sic_client

##################################################################### 
#   Options            					            #
#####################################################################

CC = gcc
CFLACS = -g -std=gnu99 -lssl -UOPENSSL_NO_EC 
LINKS = -I/usr/local/include -L/usr/local/lib -lgsl -lgslcblas -lm -lcrypto

##################################################################### 
#   Rules   				  			    #
#####################################################################

all: sic_client

sic_client: sic_client.c ../server/ecdsa.c
	$(CC) $(CFLACS) $(BINARY) ../server/ecdsa.c sic_client.c $(LINKS)

clean:
	rm -f $(BINARY) $(OBJECTS)
  
