#define BUF_LEN 65535

typedef struct 
{
    char * option;
    char * value;
} ov_pair_t;



struct radconf_t
{
	struct in_addr rad_host;
	u_int16_t rad_auth_port;
	u_int16_t rad_acct_port;
	char * rad_secret;
};

struct authdata_t 
{
	char * uname;
	struct radconf_t * radconf;
	struct rad_handle * rad_handle;
};

struct ipoed_user_profile_t
{
	u_char auth;
	u_char reauth;
	u_int32_t in_band;
	u_int32_t out_band;
	u_int16_t sess_time_out;
	u_int16_t acct_interim;
	struct authdata_t authdata;
};

struct ipoed_settings_t 
{
    u_int16_t divert_port;
    u_int16_t table_auth;
    u_int16_t table_shaping;
    u_char daemonize;
    struct radconf_t * radconf;
};
