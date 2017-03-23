#define RAD_NACK 	0
#define RAD_ACK 	1

#define RAD_ACCESS_REQUEST 	1
#define RAD_ACCESS_ACCEPT 	2
#define RAD_ACCESS_REJECT 	3

#define RAD_ACCOUNTING_REQUEST 	4
#define RAD_ACCOUNTING_RESPONSE	5

extern int radius_authenticate(struct authdata_t *);
extern void radius_close(struct  authdata_t *);

