#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXSTR 512
#define INTSTR 32

int foo(int x, int y){	
	int z = 3, a = 6,b,c,d,e;
	
	char *sbuf, *ibuf;
	
	sbuf = (char *) malloc(MAXSTR * (sizeof(char)));
	ibuf = (char *) malloc(INTSTR * (sizeof(char)));
	
	FILE *fp = fdopen(STDIN_FILENO, "r");
	
	fgets(sbuf, MAXSTR, fp);
	
	ibuf = strtok (sbuf, " ");
	b = strtol(ibuf,NULL,10);
	
	ibuf = strtok (sbuf, " ");
	c = strtol(ibuf,NULL,10);
	
	ibuf = strtok (sbuf, " ");
	d = strtol(ibuf,NULL,10);

	ibuf = strtok (sbuf, " ");
	e = strtol(ibuf,NULL,10);
	
	a = z + a;
	a = z + b;
	c = a + b;
		
	e = b + a;
	d = a / a;
	e = d + z;
	
	
	
printf("x : %d, y = %d, a = %d, b = %d, c = %d, d = %d, e = %d\n",x,y,a,b, c,d,e);
return e + 5;

}

int main(){
	int a = 6, b = 3, c, d, e, f;
	
	
	d = a + b; // a and b both constants d is constant
	f = a; // f is constant
	f = a * 0; //value of f is zero	 
	e = a / b; //e is now constant
	f = e * d;
	e = a / b;
	c = b / a; //not a duplicate expression
	e = d * e; //duplicate duplicate and constantbcz e actually does not change
	e = d;
	c = e * d; //not duplicate
	
	printf("a = %d, b = %d, c = %d, d = %d, e = %d, f = %d\n", a,b,c,d,e,f);
	e = foo(c,b);
	printf("After foo a is %d\n", e);
	return 0;	
	
}



