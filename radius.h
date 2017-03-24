#define RAD_NACK 	0
#define RAD_ACK 	1

#define RAD_ACCESS_REQUEST 	1
#define RAD_ACCESS_ACCEPT 	2
#define RAD_ACCESS_REJECT 	3

#define RAD_ACCOUNTING_REQUEST 	4
#define RAD_ACCOUNTING_RESPONSE	5

/* Limits */
#define ERRSIZE         128             /* Maximum error message length */
#define MAXCONFLINE     1024            /* Maximum config file line length */
#define MAXSERVERS      10              /* Maximum number of servers to try */
#define MSGSIZE         4096            /* Maximum RADIUS message */
#define PASSSIZE        128             /* Maximum significant password chars */


extern int radius_authenticate(struct authdata_t *);
extern int radius_account(struct authdata_t *)
extern void radius_close(struct  authdata_t *);

