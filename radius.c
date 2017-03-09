/* STD INCLUDE */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/sysctl.h>

/* Network include */

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <radlib.h>
#include <radlib_vs.h>

#include "ipoed.h"
#include "radius.h"

int rad_initialize(struct rad_handle * rad_handle, struct ipoed_settings_t * ipoed_settings, char * errmsg)
{
	int rad_port;
	int timeout = 10;
	int tries = 2;
	int errcode = -1;
	char * rad_host;
	
	rad_host = inet_ntoa(ipoed_settings->rad_srv_host);
	rad_handle = rad_auth_open();
	
	if( (errcode = rad_add_server(rad_handle, rad_host, ipoed_settings->rad_auth_port, ipoed_settings->rad_secret, timeout, tries)) == -1 )
	{
		strcpy(errmsg, "Unable to add server!\n");
		return -1;
	}
	
	if ( (errcode = rad_create_request(rad_handle, RAD_ACCESS_REQUEST)) == -1 )
	{
		strcpy(errmsg, "Unable to create ACCESS_REQUEST\n");
		return -1;
	}
	
	if ( (errcode = rad_put_addr(rad_handle, RAD_NAS_IP_ADDRESS, ipoed_settings->rad_srv_host)) == -1 )
	{
		strcpy(errmsg, "Unable to add NAS_IP_ADDRESS\n");
		return -1;
	}
	
	if ( (errcode = rad_put_int(rad_handle, RAD_NAS_PORT, ipoed_settings->rad_auth_port)) == -1 )
	{
		strcpy(errmsg, "Unable to add NAS_PORT\n");
		return -1;
	}
	return (0);
}

int rad_add_user_name(struct rad_handle * rad_handle, struct in_addr ip, char * errmsg)
{
	int errcode = -1;
	char * c_ip;
	if (rad_handle == NULL)
	{
		strcpy(errmsg, "Expected pointer to rad_handle, but NULL given\n");
		return -1;
	}
	
	/* if ( (c_ip = inet_ntoa()) == NULL)
	{
		strcpy(errmsg, "NO IP address given\n");
		return -1;
	}
	*/
	if ( (errcode = rad_put_string(rad_handle, RAD_USER_NAME, c_ip)) == -1 )
	{
		strcpy(errmsg, "Unable to add RAD_USER_NAME\n");
		return -1;
	}
	return (0);
}

int rad_send_req(struct rad_handle * rad_handle, char * errmsg)
{
	int errcode = -1;
	
	if ( (errcode = rad_send_request(rad_handle)) == -1 )
	{
		strcpy(errmsg, "Unable to send request\n");
		return -1;
	}
	return errcode;
}