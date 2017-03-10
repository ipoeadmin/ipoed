
int rad_initialize(struct rad_handle * , struct ipoed_settings_t * , char * );
int rad_add_user_name(struct rad_handle *, struct in_addr, char * );
int rad_send_req(struct rad_handle *, char *);