#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>



void cpu_exec(uint32_t);

/* We use the ``readline'' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si","step through",cmd_si },
	{ "info","print regInfo or watchPointInfo",cmd_info },
	{	"x","Scan memory",cmd_x},
	{	"p","expression evaluation",cmd_p},
	{	"w","set watchpoint",cmd_w},
	{ "d","delete watchpoint according to the number",cmd_d},

	/* TODO: Add more commands */


};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))
static int cmd_d(char *args){
	char *arg = strtok(NULL,"");
	int NO;
	if (arg == NULL) {
		printf("please input number");
	}else{
		if (sscanf(arg,"%d",&NO)==-1) {
			printf("please input correct number");
		}else{
			WP* tempH = head;
			WP* target = NULL;
			while (tempH!=NULL) {
				if(tempH->NO == NO){
					target = tempH;
					free_wp(target);
					printf("delete watchpoint NO.%d success\n",NO);
					break;
				}
				tempH = tempH->next;
			}
			if (target == NULL) {
				printf("do not have NO.%d in watchpoint pool\n",NO);
			}
		}
	}
	return 0;

}
static int cmd_w(char *args){
	char *arg = strtok(NULL,"");
	bool success = true;
	bool* successp = &success;
	if (arg == NULL) {
		printf("please input the expression\n");
	}else{
		WP* wp = new_wp();
		uint32_t result = expr(arg,successp);
		wp->result = result;
		strcpy(wp->str,arg);
		printf("set watchpoint,NO.%d; %s = %u\n",wp->NO,arg,result);
	}
	return 0;
}
static int cmd_p(char *args){//TODO:表达式求值
	char *arg = strtok(NULL,"");
	bool success = true;
	bool* successp = &success;
	if (arg == NULL) {
		printf("please input the expression\n");
	}else{

		printf("%s = %u\n",arg,expr(arg,successp));
	}
	return 0;
}
static int cmd_x(char *args){
	char *arg = strtok(NULL," "); //解析个数
	int n;
	if(arg == NULL){
		printf("please input length\n");
		return 0;
	}else if(sscanf(arg,"%d",&n)==-1){
		printf("please input correct number\n");
		return 0;
	}else{
		char *arg1 = strtok(NULL," ");//解析起始地址,TODO:表达式运算和异常的判断
		if(arg == NULL){
			printf("please input start address\n");
			return 0;
		}else{
			swaddr_t startAddress;
			sscanf(arg1,"%x",&startAddress);
			int x;
			for (x = 0; x < n; x++) {
				printf("%d.  0x%x : 0x%x\n",x+1,startAddress,swaddr_read(startAddress,4));
				startAddress += 4;
			}
		}
	}
	return 0;
}

static int cmd_info(char *args){
	//r or w
	char *arg = strtok(NULL," ");
	if(arg == NULL){
		printf("please input r or w after info\n");
		return 0;
	}
	//打印寄存器状态
	if(strcmp(arg,"r")==0){
		int i=R_EAX;
		for(;i<R_EDI;i++){
			printf(" %s : 0x%x\n",regsl[i],cpu.gpr[i]._32);
		}

	}
	//打印监视点信息
	else if (strcmp(arg,"w")==0) {
		bool success = true;
		bool* successp = &success;
		WP* tempH = head;
		while(tempH!=NULL){
			printf("watchpoint NO.%d , expr is %s = %u\n",tempH->NO,tempH->str,expr(tempH->str,successp));
			tempH = tempH->next;
		}
	}else{
		printf("Unknown Instruction\n");
	}
	return 0;
}

static int cmd_si(char *args){
	char *arg = strtok(NULL," ");
	int n;
	if(arg == NULL)
		cpu_exec(1);
	else{
		if(sscanf(arg,"%d",&n)==-1)
		{
			printf("Unknown Number\n");
		}else{
			cpu_exec(n);
		}
	}
	return 0;
}
static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
