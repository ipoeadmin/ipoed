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
#include <syslog.h>
#include <sys/param.h>
#include <poll.h>
#include <sys/time.h>
/* Network include */

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <radlib.h>
//#include <radlib_private.h>
#include <radlib_vs.h>

#include "ipoed.h"
#include "radius.h"

static int radius_open(struct authdata_t * authdata, short request_type);
static int radius_start(struct authdata_t * authdata, short request_type);
static int radius_put_auth(struct authdata_t * authdata);
static int radius_send_request(struct authdata_t * authdata);
static int radius_get_params(struct authdata_t * authdata);

void radius_close(struct authdata_t * authdata)
{
	if (authdata->rad_handle != NULL)
	    rad_close(authdata->rad_handle);
	authdata->rad_handle = NULL;
}

int radius_authenticate(struct authdata_t * authdata)
{
	syslog(LOG_INFO, "RADIUS: Authenticating user '%s'\n", authdata->uname);
	if (radius_start(authdata, RAD_ACCESS_REQUEST) == RAD_NACK || radius_put_auth(authdata) == RAD_NACK || radius_send_request(authdata) == RAD_NACK)
		return (-1);
//	authdata->status=1;
	return (0);
}

static int radius_open(struct authdata_t * authdata, short request_type)
{
	if (request_type == RAD_ACCESS_REQUEST)
	{
		
		if ((authdata->rad_handle = rad_auth_open()) == NULL)
		{
			syslog(LOG_ERR, "RADIUS: rad_auth_open() failed for '%s'\n", authdata->uname);
			return RAD_NACK;
		}
	}
	else
	{
		if ((authdata->rad_handle = rad_acct_open()) == NULL)
		{
			syslog(LOG_ERR, "RADIUS: rad_acct_open() failed for '%s'\n", authdata->uname);
			return RAD_NACK;
		}
	}
	return RAD_ACK;
}

static int radius_start(struct authdata_t * authdata, short request_type)
{
	char host[MAXHOSTNAMELEN];
	char *tmpval;
	
	if (radius_open(authdata, request_type) == RAD_NACK)
		return RAD_NACK;
		
	if (request_type == RAD_ACCESS_REQUEST)
	{
		if (authdata->radconf->rad_auth_port !=0)
		{
			syslog(LOG_INFO, "RADIUS: Adding server %s %d", inet_ntoa(authdata->radconf->rad_host), authdata->radconf->rad_auth_port);
			
			if (rad_add_server(authdata->rad_handle, inet_ntoa(authdata->radconf->rad_host), authdata->radconf->rad_auth_port, authdata->radconf->rad_secret, 10, 3) == -1)
			{
				syslog(LOG_ERR, "RADIUS: Adding server error\n");
				return RAD_NACK;
			}
		}
	}
		
	if (rad_create_request(authdata->rad_handle, request_type) == -1)
	{
		printf("rad_handle = %p\n", authdata->rad_handle);
		syslog(LOG_ERR, "RADIUS: rad_create_request() failed for '%s'\n", authdata->uname);
		printf("%s\n", rad_strerror(authdata->rad_handle));
		return RAD_NACK;
	}
	
	if (gethostname(host, sizeof(host)) == -1)
	{
		syslog(LOG_ERR, "RADIUS: gethostname() failed for RAD_NAS_IDENTIFIER\n");
		return RAD_NACK;
	}
	tmpval = host;
	
	syslog(LOG_ERR, "RADIUS: Put RAD_NAS_IDENTIFIER: '%s'\n", tmpval);
	
	if (rad_put_string(authdata->rad_handle, RAD_NAS_IDENTIFIER, tmpval) == -1)
	{
		syslog(LOG_ERR, "RADIUS: Put RAD_NAS_IDENTIFIER filed\n");
		return RAD_NACK;
	}
	
	if (authdata->me.s_addr !=0)
	{
		syslog(LOG_INFO, "RADIUS: Put RAD_NAS_IP_ADDRESS: %s", inet_ntoa(authdata->me));
		if (rad_put_addr(authdata->rad_handle, RAD_NAS_IP_ADDRESS, authdata->me) == -1)
		{
			syslog(LOG_ERR, "RADIUS: Put RAD_NAS_IP_ADDRESS failed");
			return RAD_NACK;
		}
	}
	
	if (authdata->uname == NULL)
	{
		syslog(LOG_ERR, "RADIUS: Invalid User-Name");
		return RAD_NACK;
	}
	
	syslog(LOG_INFO, "RADIUS: Put RAD_ACCT_SESSION_ID: %s", authdata->session_id);
	
	if (rad_put_string(authdata->rad_handle, RAD_ACCT_SESSION_ID, authdata->session_id) != 0)
	{
		syslog(LOG_ERR, "RADIUS: Put RAD_ACCT_SESSION_ID failed");
		return RAD_NACK;
	}
	
	syslog(LOG_INFO, "RADIUS: Put RAD_SERVICE_TYPE: RAD_FRAMED");
	if (rad_put_int(authdata->rad_handle, RAD_SERVICE_TYPE, RAD_FRAMED) == -1)
	{
		syslog(LOG_ERR, "RADIUS: Put RAD_SERVICE_TYPE failed");
		return RAD_NACK;
	}
	

	
	return RAD_ACK;
}

static int radius_put_auth(struct authdata_t * authdata)
{
	syslog(LOG_INFO, "RADIUS: Put RAD_USER_NAME: %s", authdata->uname);
	if (rad_put_string(authdata->rad_handle, RAD_USER_NAME, authdata->uname) == -1)
	{
		syslog(LOG_ERR, "RADIUS: Put RAD_USER_NAME failed");
		return RAD_NACK;
	}
	
	syslog(LOG_INFO, "RADIUS: Put RAD_USER_PASSWORD: NULL");
	if (rad_put_string(authdata->rad_handle, RAD_USER_PASSWORD, "\0") == -1)
	{
		syslog(LOG_ERR, "RADIUS: Put RAD_USER_PASSWORD failed");
		return RAD_NACK;
	}
	return RAD_ACK;
}

static int radius_send_request(struct authdata_t * authdata)
{
	struct timeval timelimit;
	struct timeval tv;
	int fd, n;
/* DEBUG
	syslog(LOG_INFO, "RADIUS: Send request for %s", authdata->uname);
*/
	n = rad_init_send_request(authdata->rad_handle, &fd, &tv);
	if ( n!=0 )
	{
		syslog(LOG_ERR, "RADIUS: rad_init_send_request failed: %d %s", n, rad_strerror(authdata->rad_handle));
		return RAD_NACK;
	}
	gettimeofday(&timelimit, NULL);
	timeradd(&tv, &timelimit, &timelimit);
	for ( ; ; )
	{
		struct pollfd fds[1];
		fds[0].fd = fd;
		fds[0].events = POLLIN;
		fds[0].revents = 123;
		n = poll(fds, 1, tv.tv_sec * 1000 + tv.tv_usec/1000);
		if (n == -1)
		{
			return RAD_NACK;
		}
		if ((fds[0].revents&POLLIN) != POLLIN)
		{
/* DEBUG		syslog(LOG_INFO, "DEBUG: not pollin %d", fds[0].revents); */
			gettimeofday(&tv, NULL);
			timersub(&timelimit, &tv, &tv);
			if (tv.tv_sec > 0 || ((tv.tv_sec == 0) && (tv.tv_usec > 0) ))
				continue;
		}
		
		syslog(LOG_INFO, "RADIUS: Sending request for user '%s'", authdata->uname);
/* FOR DEBUG
		syslog(LOG_INFO, "DEBUG: rad_host from rad_handle = %s", inet_ntoa(authdata->rad_handle->servers[0].addr.sin_addr));
		syslog(LOG_INFO, "DEBUG: rad_secret from rad_handle = %s", authdata->rad_handle->servers[0].secret);
		syslog(LOG_INFO, "DEBUG: rad_auth_port from rad_handle = %d", authdata->rad_handle->servers[0].secret);
		syslog(LOG_INFO, "DEBUG: rad_handle after is  %p", authdata->rad_handle);
*/
		n = rad_continue_send_request(authdata->rad_handle, n, &fd, &tv);
		if (n !=0)
			break;
		
		gettimeofday(&timelimit, NULL);
		timeradd(&tv, &timelimit, & timelimit);
	}
	
	switch (n)
	{
		case RAD_ACCESS_ACCEPT: 
			syslog(LOG_INFO, "Recieved RAD_ACCESS_ACCEPT for user '%s'", authdata->uname);
			authdata->status = 1;
			break;

		case RAD_ACCESS_CHALLENGE:
			syslog(LOG_INFO, "Recieved RAD_ACCESS_CHALLENGE for user '%s'", authdata->uname);
			break;
			
		case RAD_ACCESS_REJECT:
			syslog(LOG_INFO, "Recieved RAD_ACCESS_REJECT for user '%s'", authdata->uname);
			authdata->status = 0;
			break;
		case RAD_ACCOUNTING_RESPONSE:
			break;
		
		case -1:
			syslog(LOG_ERR, "RADIUS: rad_send_request for user '%s' filed: %s", authdata->uname, rad_strerror(authdata->rad_handle));
			break;
			return RAD_NACK;
		
		default:
			syslog(LOG_ERR, "RADIUS: rad_send_request: unexpected returned value: %d", n);
			return RAD_NACK;
		
	}
	
	return (radius_get_params(authdata));
}

static int radius_get_params(struct authdata_t * authdata)
{
	const void * data;
	size_t len;
	int res, i, j;
	u_int32_t timeout;
	u_int32_t vendor;
	char buf[64];
	struct in_addr ip;
	
	while (( res = rad_get_attr(authdata->rad_handle, &data, &len)) > 0 )
	{
		switch (res)
		{
			case RAD_FRAMED_PROTOCOL:
				i = rad_cvt_int(data);
				syslog(LOG_INFO, "RADIUS: RAD_FRAMED_PROTOCOL=%d", i);
				break;
				
			case RAD_SERVICE_TYPE:
				i = rad_cvt_int(data);
				syslog(LOG_INFO, "RADIUS: RAD_SERVICE_TYPE=%d", i);
				break;
				
			case RAD_FRAMED_IP_ADDRESS:
				ip = rad_cvt_addr(data);
				syslog(LOG_INFO, "RADIUS: RAD_FRAMED_IP_ADDRESS=%s", inet_ntoa(ip));
				break;
				
			case RAD_FRAMED_IP_NETMASK:
				ip = rad_cvt_addr(data);
				syslog(LOG_INFO, "RADIUS: RAD_FRAMED_IP_NETMASK=%s", inet_ntoa(ip));
				break;
				
			case RAD_SESSION_TIMEOUT:
				timeout = rad_cvt_int(data);
				syslog(LOG_INFO, "RADIUS: RAD_SESSION_TIMEOUT=%u", timeout);
				break;
				
			case RAD_ACCT_INTERIM_INTERVAL:
				timeout = rad_cvt_int(data);
				syslog(LOG_INFO, "RADIUS: RAD_ACCT_INTERIM_INTERVAL=%u", timeout);
				break;
				
			case RAD_VENDOR_SPECIFIC:
				if (( res = rad_get_vendor_attr(&vendor, &data, &len)) == -1)
					{
						syslog(LOG_ERR, "RADIUS: Get vendor attr failed: %s", rad_strerror(authdata->rad_handle));
						return RAD_NACK;
					}
				switch (vendor)
				{
					case 9:
						switch (res)
						{
							case 1: 
								syslog(LOG_INFO, "RADIUS: Get Cisco-AVPair: '%s'", rad_cvt_string(data, len));
								break;
						}
				}
		}
	}
	return RAD_ACK;
}
