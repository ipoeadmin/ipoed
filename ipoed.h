#DEFINE BUF_LEN 65535;



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
