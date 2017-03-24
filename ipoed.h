#define BUF_LEN 65535
#define ACL_NAME_LEN 255

#define DEBUG

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

struct acl_t 
{
	struct acl_t * next;
	char name[ACL_NAME_LEN];
	char * rule;
};


struct authdata_t 
{
	char * uname;
	struct in_addr me;
	u_char status;
	struct timeval * session_updated;
	char * session_id;
	u_int32_t sess_time_out;
	u_int32_t acct_interim;
	struct radconf_t * radconf;
	struct rad_handle * rad_handle;
	struct acl_t * acl;
};

struct ipoed_user_profile_t
{	
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
