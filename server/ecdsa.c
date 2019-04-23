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
#include <string.h>
#include <stdlib.h>
#include <openssl/ec.h>      // for EC_GROUP_new_by_curve_name, EC_GROUP_free, EC_KEY_new, EC_KEY_set_group, EC_KEY_generate_key, EC_KEY_free
#include <openssl/ecdsa.h>   // for ECDSA_do_sign, ECDSA_do_verify
#include <openssl/obj_mac.h> // for NID_secp256k1
#include <openssl/pem.h>

EC_KEY *load_pub_key(char *PUBKEYFILE)
{
    EC_KEY *eckey; // private key
    FILE *f = fopen(PUBKEYFILE,"r");
    if (f!=NULL)
    {
    	eckey = PEM_read_EC_PUBKEY(f,NULL,NULL,NULL);
    	fclose(f);
        printf("Loaded public key from %s\n",PUBKEYFILE);
	return eckey;
    }
    return NULL;
}


EC_KEY *load_key(char *PRIVKEYFILE,char *PUBKEYFILE)
{
    EC_KEY *eckey; // private key
    int function_status = -1;
    FILE *f = fopen(PRIVKEYFILE,"r");
    if (f!=NULL)
    {
    	eckey = PEM_read_ECPrivateKey(f,NULL,NULL,NULL);
    	fclose(f);
    	//if (EC_KEY_check_key(eckey))
        printf("Loaded private key from %s\n",PRIVKEYFILE);
        function_status = 1;
	return eckey;
    }
    else
    {
        eckey=EC_KEY_new();
        EC_GROUP *ecgroup= EC_GROUP_new_by_curve_name(NID_secp256k1);
        if (NULL == ecgroup)
        {
            printf("Failed to create new EC Group\n");
            function_status = -1;
        }
        else
        {
            int set_group_status = EC_KEY_set_group(eckey,ecgroup);
            const int set_group_success = 1;
            if (set_group_success != set_group_status)
            {
                printf("Failed to set group for EC Key\n");
                function_status = -1;
            }
            else
            {
                const int gen_success = 1;
                int gen_status = EC_KEY_generate_key(eckey);
                if (gen_success != gen_status)
                {
                    printf("Failed to generate EC Key\n");
                    function_status = -1;
                }
                else
                {
		    FILE * f = fopen(PUBKEYFILE,"w");
                    PEM_write_EC_PUBKEY(f, eckey);
        	    fclose(f);
		    f = fopen(PRIVKEYFILE,"w");
		    PEM_write_ECPrivateKey(f,eckey, NULL,NULL,0,NULL,NULL);
        	    fclose(f);
		    printf("Saved new public key to %s\n",PUBKEYFILE);
        	    printf("Saved new private key to %s\n",PRIVKEYFILE);
		    return(eckey);
		}
	}
	}
	}
 }
