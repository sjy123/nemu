#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;
	char str[32]; //表达式
	uint32_t result; //监视值
	/* TODO: Add more members if necessary */


} WP;
WP* new_wp();
void free_wp(WP *wp);
#endif
