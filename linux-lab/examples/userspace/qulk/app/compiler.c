#include <stdio.h>
#include "compiler.h"

//OVS_NO_RETURN int c_no_return(int a, int b);
OVS_UNUSED int c_unused(int a, int b);
OVS_PRINTF_FORMAT(1, 2) int my_printf(const char *, ...);

/* 
OVS_NO_RETURN void no_return(int a, int b)
{
	return a+b;
}
*/

OVS_UNUSED int c_unused(int a, int b)
{
	return a+b;
}
OVS_PRINTF_FORMAT(1, 2) int my_printf(const char *fmt, ...)
{
	return 0;
}

/* gcc -O2 */
int c_likely(int a, int b)
{
	if (OVS_LIKELY(a==2 && b == 3))
	{
		a+= 2;
		b+= 2;
	}

	printf("a: %d, b: %d\n", a, b);

	return a+b;
}

int c_name_constructor(int a, int b)
{
	int NAMECTR(test) = a+b;
	int NAMECTR(test) = a+b;
	int NAMECTR(test) = a+b;

	printf("%s: test0 %d, test1, %d, test2 %d, %s, %s, %s\n", 
			__func__, test0, test1, test2, STR(test0), STR(test1), STR(test2));
	printf("%s: %s, %s, %s\n", __func__, STR(NAMECTR(test)), STR(NAMECTR(test)), STR(NAMECTR(test)));

	return 0;
}

int c_macro_variant(int a, int b)
{
	printf("%s: %s, %s, %s, %s\n", __func__, 
			STR(MACRO_REPEAT(test, a)), STR(MACRO_REPEAT(test, b)), STR(MACRO_REPEAT(test, a, b)), STR(MACRO_REPEAT(test, a, b, c, d)));
	return 0;
}

int c_map_list(int a, int b)
{
	printf("%s: %s, %s, %s, %s\n", __func__, 
			STR(MAP_LISTS(a)), STR(MAP_LISTS(b)), STR(MAP_LISTS(a, b)), STR(MAP_LISTS(a, b, c, d, e, f)));
	return 0;
}

int c_pp_narg(int a, int b)
{
	printf("%s: %s, %s, %s %s\n", __func__,
			STR(PP_NARG(a)), STR(PP_NARG(b)), STR(PP_NARG(a, b)), STR(PP_NARG(a, b, c, d, e, f)));
	return 0;
}

int main(int argc, char **argv)
{
	//no_return(1, 2);
	//
	c_likely(3, 4);
	c_name_constructor(3, 4);
	c_macro_variant(3, 4);
	c_map_list(3, 4);
	c_pp_narg(3, 4);
}
