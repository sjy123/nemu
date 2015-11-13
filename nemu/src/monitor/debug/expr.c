#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ
	,REG,INT_10,INT_16

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{"0[xX][0-9a-fA-F]+", INT_16},//0xfffffff 等16进制数字匹配
	{"\\d+",INT_10}, //10进制整数匹配
	{"\\$%[a-z]+", REG}, //$eax等寄存器匹配
	{" +",	NOTYPE},				// spaces
	{"\\+", '+'},					// plus
	{"-", '-'}, // sub or 负号
	{"/", '/'}, //div
	{"\\*", '*'}, //multi or 地址
	{"\\(",'('}, //LB 左括号
	{"\\)",')'}, //RB 右括号
	{"==", EQ}						// equal
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

uint32_t eval(uint32_t p,uint32_t q); //计算函数
bool check_parentheses(uint32_t p,uint32_t q) ; //判断括号
uint32_t getOp(uint32_t p,uint32_t q); //求出关键点位置
/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;

	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain
				 * types of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE:break;
					case INT_16:
						tokens[nr_token].type = rules[i].token_type;
						memcpy(tokens[nr_token].str,substr_start,substr_len);
						nr_token++;
						break;
					case INT_10:
						assert(substr_len<32);
						tokens[nr_token].type = rules[i].token_type;
						memcpy(tokens[nr_token].str,substr_start,substr_len);
						nr_token++;
						break;
					case '+':
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;
						break;
					case '-':
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;
						break;
					case '*':
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;
						break;
					case '/':
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;
						break;
					case REG:
						tokens[nr_token].type = rules[i].token_type;
						memcpy(tokens[nr_token].str,substr_start,substr_len);
						nr_token++;
						break;
					case '(':
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;
						break;
					case ')':
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;
						break;
					default: panic("please implement me");
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
		assert(nr_token<32); //token数量限制
	}

	return true;
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}
	/* TODO: Insert codes to evaluate the expression. */
	return eval(0,nr_token-1);
}
uint32_t eval(uint32_t p,uint32_t q){
	assert(p<=q);
	if (p==q) { //解析数字

		uint32_t n;
		if(tokens[q].type == INT_16)
			sscanf(tokens[q].str,"%x",&n);
		else sscanf(tokens[q].str,"%d",&n);
		return n;

	}else if(check_parentheses(p,q) == true){ //判断括号
			return eval(p+1,q-1);
	}else{ //递归计算
			uint32_t op = getOp(p,q); //获取dominat operator
			uint32_t val1 = eval(p,op-1);
			uint32_t val2 = eval(op+1,q);

			switch(tokens[op].type){
				case '+':return val1+val2;
				case '-':return val1-val2;
				case '*':return val1*val2;
				case '/':return val1/val2;
				default: assert(0);
			}
	}
}

bool check_parentheses(uint32_t p,uint32_t q){
	int parentheses = 0;
	for (; p<=q; p++) {

		if (parentheses<0) {
			return false;
		}

		if (tokens[p].type == '(') {
			parentheses++;
		}else if(tokens[q].type == ')'){
			parentheses--;
		}
	}

	if (parentheses == 0) {
		return true;
	}
	return false;
}
uint32_t getOp(uint32_t p,uint32_t q){
	int parentheses = 0;
	int op=p;
	int priority=2;// p"-,+"=0,p"*,/"=1
	for (; p <= q; p++) {

		if (tokens[p].type == INT_10 || tokens[p].type == INT_16) {
			//跳过数字
			continue;
		}else if (tokens[p].type == '(') {
			//跳过括号
			parentheses++;p++;
			while(parentheses!=0){

				if (tokens[p].type == '(') {
					parentheses++;
				}else if (tokens[p].type == ')') {
					parentheses--;
				}
				p++;
			}
			p--;
		}else if (tokens[p].type == '-' || tokens[p].type == '+') {
			if (priority>=0) { //直接定位为op
				op = p ;
				priority = 0;
			}
		}else if (tokens[p].type == '*' || tokens[p].type == '/') {
			if (priority>=1) {
				op = p;
				priority = 1;
			}
		}
	}
	assert(op!=p);
	return op;
}
