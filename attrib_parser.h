#define ACL_TYPE_RATE_LIMIT_TOK 1
#define ACL_TYPE_ACCESS_GROUP_TOK 2
#define ACL_BPS 3
#define ACL_CONFORM_ACTION_TOK 4
#define ACL_ACTION 5
#define ACL_EXCEED_ACTION_TOK 6
#define ACL_RATE_LIMIT_DIRECTION 7
#define ACL_UNKNOWN 255

#define ACL_ACTION_TRANSMIT 1
#define ACL_ACTION_DROP	    2

#define ACL_DIRECTION_INPUT 1
#define ACL_DIRECTION_OUTPUT 2

struct elem_t
{
	unsigned short type;
	char content[100];
	struct elem_t * next;
};

struct acl_rule_t 
{
	u_int16_t direction;
	u_int32_t bps;
	u_int32_t n_burst;
	u_int32_t e_burst;
	u_char    conform_action;
	u_char    exceed_action;
	struct acl_rule_t * acl_rule;
};

int parse_cisco_av_pair(char * attribute, struct elem_t * elem);
int fill_acl_rule(struct elem_t * elem ,struct acl_rule_t * acl_rule);
void clear_acl_rule(struct acl_rule_t * acl_rule);

