#define BUF_LEN 65535

typedef struct 
{
    char * option;
    char * value;
} ov_pair_t;

struct ipoed_client_t
{
	char auth;
	int in_band;
	int out_band;
	int sess_time_out;
	int acct_interim;
};

struct ipoed_settings_t 
{
    u_int16_t divert_port;
    struct in_addr rad_srv_host;
    u_int16_t rad_auth_port;
    u_int16_t rad_acct_port;
    char * rad_secret;
    u_int16_t table_auth;
    u_int16_t table_shaping;
    u_char daemonize;
};
