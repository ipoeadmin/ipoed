/* STD Include */

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <sys/param.h>
#include <libutil.h>

#include <sys/time.h>

/* Net include */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

/* Syslog include */

#include <syslog.h>
#include <stdarg.h>

/* RADIUS LIBRARY */

#include <radlib.h>
#include <radlib_vs.h>

/* LOCAL INCLUDE */

#include "ipoed.h"
#include "radius.h"
#include "attrib_parser.h"

/* Global variables definitions */

int sock;
int running = 1;
char * errmsg;
int errcode;
u_char daemonize = 0;

/* End of global variables definition */



static const char * pid_name = "var/run/ipoed.pid";

/* Some Function definitions */

static void Shutdown(int sig __unused);
int parse_args(ov_pair_t ** ov_pair, char ** argv, int argc, u_char * daemonize,char * errmsg);
int init_settings(struct ipoed_settings_t * ipoed_settings, ov_pair_t ** ov_pair, int argc, char * errmsg);

int is_valid_port_number(int port_num);
int is_valid_ip_address(struct in_addr * ip);
int is_bit(int bit);

static void daemon_mode(void);

static int get_session_id(char * ip, char * res);

void print_settings(struct ipoed_settings_t * ipoed_settings);

int main(int argc, char ** argv)
{
    /* Global settings*/
    
    struct timeval * tv;
    
    struct ipoed_settings_t ipoed_settings =
    {
	.divert_port = htons(9000),
	.table_auth = htons(1),
	.table_shaping = htons(2),
	.daemonize = 0
    };
	
    /* Option value pair */
	
    ov_pair_t ** ov_pair;
    
    /* For packet processing */
    
    struct sockaddr_in addr;
    struct ip * ip;
    socklen_t addr_size;
    int orig_byte;
    char buf[BUF_LEN];
	
    struct in_addr ip_in_addr;
    unsigned short ip_hash = 0;
	
    struct radconf_t * radconf;
    struct ipoed_user_profile_t ipoed_user_profiles[65535];
    
    openlog("ipoed", LOG_PID | LOG_NDELAY | LOG_CONS | LOG_PERROR, LOG_USER);
    errmsg = (char *)malloc(sizeof errmsg);
    
    ov_pair = (ov_pair_t **)malloc(sizeof(ov_pair_t *) * argc);
    
    radconf = (struct radconf_t *)malloc(sizeof(struct radconf_t));
    
    radconf->rad_host.s_addr = inet_addr("127.0.0.1");
    radconf->rad_auth_port = 1812;
    radconf->rad_acct_port = 1813;
    
    radconf->rad_secret = (char *)malloc(sizeof(char) * 255);
    
    ipoed_settings.radconf = radconf;
	
    syslog (LOG_INFO, "Attemtping to parse_args()...");
    errcode = parse_args(ov_pair, argv, argc, &daemonize, errmsg);
    
    if (errcode)
    {
	syslog(LOG_ERR, "Parse error: %s", errmsg);
	return -1;
    }
    syslog (LOG_INFO, "parse_args() done.");
    syslog (LOG_INFO, "Attempting to init_settings()...");
    errcode = init_settings(&ipoed_settings, ov_pair, argc, errmsg);
    
    if (errcode)
    {
	syslog(LOG_ERR, "Init error: %s", errmsg);
	return -1;
    }
    
    syslog (LOG_INFO, "init_settings() done.\n");
    print_settings(&ipoed_settings);
    
    if (ipoed_settings.daemonize)
    {
        syslog(LOG_INFO, "Daemonize...");
        daemon_mode();
    }
	
    /* Bind divert socket */
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = ipoed_settings.divert_port;
    addr_size = sizeof addr;

    if((sock = socket(PF_INET, SOCK_RAW, IPPROTO_DIVERT)) == -1)
    {
  	   strcpy(errmsg, "Unable to create global divert socket\n");
	   syslog(LOG_ERR, "Divert error: %s", errmsg);
	   return -1;
    };
	
    errcode = bind(sock, (struct sockaddr *) &addr, addr_size);
    if (errcode)
    {
	    strcpy(errmsg, "Unable to bind global divert socket\n");
	    syslog(LOG_ERR, "Divert error: %s", errmsg);
	    return -1;
    };
	
    siginterrupt(SIGTERM, 1);
    signal(SIGTERM, Shutdown);
	
    /* Packet processing */	
	
    while (running)
    {
	    orig_byte = recvfrom(sock, buf, BUF_LEN, 0, (struct sockaddr *) &addr, &addr_size);
	    if (orig_byte == -1)
	    {
		    if (errno != EINTR)
			    strcpy(errmsg, "Read from divert socket failed\n");
		    syslog(LOG_ERR, "Divert error: %s\n", errmsg);
	    }
	    ip = (struct ip*) buf;
	    ip_in_addr = ip->ip_src;
	    ip_hash = htonl(ip_in_addr.s_addr) & 0x0000ffff;
	    ipoed_user_profiles[ip_hash].authdata.radconf = ipoed_settings.radconf;
	    ipoed_user_profiles[ip_hash].authdata.rad_handle = rad_open();
	    
	    if (ipoed_user_profiles[ip_hash].authdata.session_updated == NULL)
		ipoed_user_profiles[ip_hash].authdata.session_updated = (struct timeval *)malloc(sizeof(struct timeval));
		
	    if (ipoed_user_profiles[ip_hash].authdata.uname == NULL)
		ipoed_user_profiles[ip_hash].authdata.uname = (char *)malloc(sizeof(char) * 16);

	    strcpy(ipoed_user_profiles[ip_hash].authdata.uname, inet_ntoa(ip_in_addr));

	    if (ipoed_user_profiles[ip_hash].authdata.session_id == NULL)
		ipoed_user_profiles[ip_hash].authdata.session_id = (char *)malloc(sizeof(char) * 20);

	    get_session_id(ipoed_user_profiles[ip_hash].authdata.uname, ipoed_user_profiles[ip_hash].authdata.session_id);
	    
	    /* Check auth status */
	
	    if (ipoed_user_profiles[ip_hash].authdata.status == 1)
		continue;

	    radius_authenticate(&(ipoed_user_profiles[ip_hash].authdata));
		
	    radius_close(&(ipoed_user_profiles[ip_hash].authdata));
    }
	
    /* END of Packet processing */
	
    free(errmsg);
    free(ipoed_settings.radconf);
    free(radconf);
    free(ov_pair);
    free(&ipoed_user_profiles);
    closelog();
    return (0);
}

/* Function implementation */

static void daemon_mode(void)
{
    FILE * pid_file;
    
    daemon (0, 0);
    pid_file = fopen(pid_name, "w");
    if (pid_file)
    {
	fprintf(pid_file, "%d\n", getpid());
	fclose(pid_file);
    }
    
}

void print_settings(struct ipoed_settings_t * ipoed_settings)
{
    printf("Divert port: %d\n", ntohs(ipoed_settings->divert_port));
    printf("RADIUS Server IP address: %s\n", inet_ntoa(ipoed_settings->radconf->rad_host));
    printf("RADIUS Auth port: %d\n", ipoed_settings->radconf->rad_auth_port);
    printf("RADIUS Acct port: %d\n", ipoed_settings->radconf->rad_acct_port);
    printf("RADIUS Secret: %s\n", ipoed_settings->radconf->rad_secret);
    printf("IPFW Auth table: %d\n", ntohs(ipoed_settings->table_auth));
    printf("IPFW Shaping table: %d\n", ntohs(ipoed_settings->table_shaping));
    printf("Daemonize: %d\n", ipoed_settings->daemonize);
}

int is_valid_port_number(int port_num)
{
    return ((port_num < 1) || (port_num > 65535)) ? 0 : 1 ;
}

int is_bit(int bit)
{
    return ((bit == 0) || (bit == 1)) ? 1 : 0 ;
}

int parse_args(ov_pair_t ** ov_pair,char ** argv, int argc, u_char * daemonize, char * errmsg)
{
    int arg;
    int ovp_index = 1;
    char sep[10] = "=";
    char * option_buf;
    char * value_buf;
    option_buf = (char *)malloc(sizeof option_buf * 100);
    value_buf = (char *)malloc(sizeof value_buf * 100);
    for (arg = 1; arg < argc; arg++)
    {
	if ((ov_pair[arg] = (ov_pair_t *)malloc(sizeof(ov_pair_t))) == NULL)
	{
	    strcpy(errmsg, "Can't allocate memory! \n");
	    return 1;
	}
	if (!(argv[arg][0] == '-') || !(argv[arg][1] == '-'))
	{
	    strcpy(errmsg, "Illegal option: ");
	    strcat(errmsg, argv[arg]);
	    strcat(errmsg, "\n");
	    return 2;
	}
	
	strcpy(value_buf, argv[arg]);
	value_buf = strpbrk(value_buf, sep);
	if ((value_buf == NULL) || (strlen(value_buf) < 2))
	{
	    strcpy(errmsg, "Illegal option value! \n");
	    return 3;
	}
	ov_pair[arg]->option = (char *)malloc(sizeof(char) * 100);
	ov_pair[arg]->value = (char *)malloc(sizeof(char) * 100);

	strcpy(option_buf, strtok(argv[arg], sep));
	strcpy(ov_pair[arg]->option, option_buf + 2);
	strcpy(ov_pair[arg]->value, value_buf + 1);
//	ovp_index++;
    }
    free(option_buf);
    free(value_buf);
    return 0;
}

int init_settings(struct ipoed_settings_t * ipoed_settings, ov_pair_t ** ov_pair, int argc, char * errmsg)
{
    int arg;
    int int_buf;
    char * char_buf;
    in_addr_t ip_addr_buf;
    char_buf=(char *)malloc(sizeof char_buf);
    if (ipoed_settings == NULL)
    {
	strcpy(errmsg, "No ipoed_settings given! \n");
	return -1;
    }
    
    for (arg = 1; arg < argc; arg++)
    {
	if (ov_pair[arg] == NULL)
	    break;
	if (!strcmp(ov_pair[arg]->option, "divert-port"))
	{
	    strcpy(char_buf, ov_pair[arg]->value);
	    if (!is_valid_port_number(int_buf = strtol(char_buf, NULL, 10)))
	    {
		strcpy(errmsg, "Divert port number is not integer or out of range\n");
		return -1;
	    }
	    ipoed_settings->divert_port = htons(int_buf);
	    continue;
	}
	
	if (!strcmp(ov_pair[arg]->option, "rad-srv-host"))
	{
	    ip_addr_buf = inet_addr(ov_pair[arg]->value);
	    if ((ip_addr_buf == INADDR_ANY) || (ip_addr_buf == INADDR_NONE))
	    {
		strcpy(errmsg, "Invalid RADIUS Server IP address!\n");
		return -1;
	    }
	    ipoed_settings->radconf->rad_host.s_addr = ip_addr_buf;
	    continue;
	}
	
	if (!strcmp(ov_pair[arg]->option, "rad-auth-port"))
	{
	    strcpy(char_buf, ov_pair[arg]->value);
	    if (!is_valid_port_number(int_buf = strtol(char_buf, NULL, 10)))
	    {
		strcpy(errmsg, "RADIUS Auth port number is not integer or out of range\n");
		return -1;
	    }
	    ipoed_settings->radconf->rad_auth_port = int_buf;
	    continue;
	}
	
	if (!strcmp(ov_pair[arg]->option, "rad-acct-port"))
	{
	    strcpy(char_buf, ov_pair[arg]->value);
	    if (!is_valid_port_number(int_buf = strtol(char_buf, NULL, 10)))
	    {
		strcpy(errmsg, "RADIUS Acct port number is not integer or out of range\n");
		return -1;
	    }
	    ipoed_settings->radconf->rad_acct_port = int_buf;
	    continue;
	}
	
	if (!strcmp(ov_pair[arg]->option, "ipfw-auth-table"))
	{
	    strcpy(char_buf, ov_pair[arg]->value);
	    if (!is_valid_port_number(int_buf = strtol(char_buf, NULL, 10)))
	    {
		strcpy(errmsg, "IPFW AUTH table number is not integer or out of range\n");
		return -1;
	    }
	    ipoed_settings->table_auth = htons(int_buf);
	    continue;
	}
	
	if (!strcmp(ov_pair[arg]->option, "ipfw-shaping-table"))
	{
	    strcpy(char_buf, ov_pair[arg]->value);
	    if (!is_valid_port_number(int_buf = strtol(char_buf, NULL, 10)))
	    {
		strcpy(errmsg, "IPFW SHAPING table number is not integer or out of range\n");
		return -1;
	    }
	    ipoed_settings->table_shaping = htons(int_buf);
	    continue;
	}
	
	if (!strcmp(ov_pair[arg]->option, "daemonize"))
	{
	    strcpy(char_buf, ov_pair[arg]->value);
	    if (!is_bit(int_buf = strtol(char_buf, NULL, 10)))
	    {
		strcpy(errmsg, "Value of daemonize must be 0 or 1 \n");
		return -1;
	    }
	    ipoed_settings->daemonize = int_buf;
	    continue;
	}
	
	if (!strcmp(ov_pair[arg]->option, "rad-secret"))
	{
	    //ipoed_settings->radconf->rad_secret = (char *)malloc(sizeof(char) * 255);
	    strcpy(ipoed_settings->radconf->rad_secret, ov_pair[arg]->value);
	    continue;
	}
	
	strcpy(errmsg, "Unknown option: ");
	strcat(errmsg, ov_pair[arg]->option);
	return -1;
    }
    free(char_buf);
    return (0);
}

static void Shutdown(int sig __unused)
{
	running = 0;
};

static int  get_session_id(char * ip, char * res)
{
        char * tmpval;
        u_int16_t ip_hash;
        in_addr_t in_addr;
        struct timeval  tv;
	tmpval = (char *)malloc(sizeof(char));
        if ((in_addr = inet_addr(ip)) == -1)
        {
                return -1;
        }
        ip_hash = in_addr & 0x0000FFFF;
	gettimeofday(&tv, NULL);
        sprintf(tmpval, "LINK-%d-%ld", ip_hash, tv.tv_sec);
        strcpy(res, tmpval);
        free(tmpval);
        return 0;
}
