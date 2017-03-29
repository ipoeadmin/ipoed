#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "attrib_parser.h"

int parse_cisco_av_pair(char * attribute, struct elem_t * elem);
int fill_acl_rule(struct elem_t * elem ,struct acl_rule_t * acl_rule);
void clear_acl_rule(struct acl_rule_t * acl_rule);

int parse_cisco_av_pair(char * attribute, struct elem_t * elem)
{
	char * input, * output;
	char delim_eq[2] = "=\0";
	char delim_sp[2] = " \0";
	struct elem_t * elem_tmp;
	input = (char * )malloc(sizeof(char)*strlen(attribute));
	strcpy(input, attribute);
	strtok_r(input, delim_eq, &output);
	if (strcmp(input, "lcp:interface") && (input != NULL))
	{
		printf("Not correct input attribute!\n");
		return -1;
	}
	elem_tmp = elem;
	while (output != NULL)
	{
		strcpy(input, output);
		strtok_r(input, delim_sp, &output);
		elem_tmp->next = (struct  elem_t *)malloc(sizeof(struct elem_t));
		strcpy(elem_tmp->content, input);
		if (!strcmp(elem_tmp->content, "rate-limit"))
			elem_tmp->type = ACL_TYPE_RATE_LIMIT_TOK;
		if ((strtol(elem_tmp->content, NULL, 10)) || !strcmp(elem_tmp->content, "0"))
			elem_tmp->type = ACL_BPS;
		if (!strcmp(elem_tmp->content, "conform-action"))
			elem_tmp->type = ACL_CONFORM_ACTION_TOK;
		if (!strcmp(elem_tmp->content, "transmit") || !strcmp(elem_tmp->content, "drop"))
			elem_tmp->type = ACL_ACTION;
		if (!strcmp(elem_tmp->content, "exceed-action"))
			elem_tmp->type = ACL_EXCEED_ACTION_TOK;
		if (elem_tmp->type == NULL)
			elem_tmp->type = ACL_UNKNOWN;
		if (!strcmp(elem_tmp->content, "input") || !strcmp(elem_tmp->content, "output"))
			elem_tmp->type = ACL_RATE_LIMIT_DIRECTION;
		elem_tmp = elem_tmp->next;
	}
	return 0;
}

int fill_acl_rate_limit_rule(struct elem_t * elem ,struct acl_rule_t * acl_rule)
{
	struct elem_t * elem_tmp;
	int i,j;
	if (!(elem->type == ACL_TYPE_RATE_LIMIT_TOK))
		return 1;
	elem_tmp = elem->next;
	
	if (!(elem_tmp->type == ACL_RATE_LIMIT_DIRECTION))
		return 2;
	
	(strcmp(elem_tmp->content, "input") == 0) ? (acl_rule->direction = ACL_DIRECTION_INPUT) : (acl_rule->direction = ACL_DIRECTION_OUTPUT);
	
	elem_tmp = elem_tmp->next;
	
	for (i = 0 ; i < 3 ; i++)
	{
		if (!(elem_tmp->type == ACL_BPS))
			return 3;
		switch (i)
		{
			case 0:
				acl_rule->bps = strtol(elem_tmp->content, NULL, 10);
			case 1:
				acl_rule->n_burst = strtol(elem_tmp->content, NULL, 10);
			case 2:
				acl_rule->e_burst = strtol(elem_tmp->content, NULL, 10);
		}
		elem_tmp = elem_tmp->next;
	};
	
	if (!(elem_tmp->type == ACL_CONFORM_ACTION_TOK))
		return 4;
	elem_tmp = elem_tmp->next;
	
	if (!(elem_tmp->type == ACL_ACTION))
		return 5;
	elem_tmp = elem_tmp->next;
	(strcmp(elem_tmp->content, "transmit") == 0) ? (acl_rule->conform_action = ACL_ACTION_TRANSMIT) : (acl_rule->conform_action = ACL_ACTION_DROP);

	if (!(elem_tmp->type == ACL_EXCEED_ACTION_TOK))
		return 6;
	
	elem_tmp = elem_tmp->next;
	
	if (!(elem_tmp->type == ACL_ACTION))
		return 7;
	
	if (elem_tmp->next != NULL)
		return 8;
	
	elem_tmp = elem;
	
	while (elem_tmp->next != NULL)
	{
		
	}
	return 0;
}

void clear_rate_limit_rule(struct acl_rule_t * acl_rule)
{
	acl_rule->direction = 0;
	acl_rule->bps = 0;
	acl_rule->n_burst = 0;
	acl_rule->e_burst = 0;
	acl_rule->conform_action = 0;
	acl_rule->exceed_action = 0;
}
