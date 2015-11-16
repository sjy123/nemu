#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_list[NR_WP];
WP *head;
static WP *free_;

void init_wp_list() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_list[i].NO = i;
		wp_list[i].next = &wp_list[i + 1];
	}
	wp_list[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_list;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(){
	//free中删除
	WP* temp;
	if (free_ == NULL) {
		Assert(0,"do not have enough watchpoint");
	}

	temp = free_;
	free_ = free_ -> next;
	//head中插入
	WP* tempH = head;
	if (head == NULL) {
		head = temp;
		temp->next = NULL;
	}else	{
		while (tempH->next!=NULL) {
			tempH = tempH->next;
		}
		tempH->next = temp;
		temp->next = NULL;
	}
	return temp;
}

void free_wp(WP *wp){
	//从head中删除
	assert(wp!=NULL);
	WP* tempH = head;
	if (tempH == wp) {
		head = head->next;
	}else{
		while(tempH!=NULL){
			if (wp == tempH->next) {
				tempH->next = tempH->next->next;
			}
			tempH = tempH->next;
		}
	}
	//将wp加入free_
	WP* tempF = free_;
	if (free_==NULL) {
		free_ = wp;
	}else{
		while (tempF->next!=NULL) {
			tempF = tempF->next;
		}
		tempF->next = wp;
		wp->next = NULL;
	}

}
