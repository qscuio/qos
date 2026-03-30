#include <stdbool.h> // For `true` (`1`) and `false` (`0`) macros in C
#include <stdint.h>  // For `uint8_t`, `int8_t`, etc.
#include <stdio.h>   // For `printf()`

#define CONCAT_(prefix, suffix) prefix##suffix
/// Concatenate `prefix, suffix` into `prefixsuffix`
#define CONCAT(prefix, suffix) CONCAT_(prefix, suffix)
/// Make a unique variable name containing the line number at the end of the
/// name. Ex: `uint64_t MAKE_UNIQUE_VARIABLE_NAME(counter) = 0;` would
/// produce `uint64_t counter_7 = 0` if the call is on line 7!
#define MAKE_UNIQUE_NAME1(prefix) CONCAT(prefix##_, __LINE__)

#define MAKE_UNIQUE_NAME2(prefix) CONCAT(prefix##_, __COUNTER__)

#define MAKE_UNIQUE_VARIABLE_NAME1(prefix) MAKE_UNIQUE_NAME1(prefix)
#define MAKE_UNIQUE_VARIABLE_NAME2(prefix) MAKE_UNIQUE_NAME2(prefix)

#define MAKE_UNIQUE_FUNCTION_NAME2(prefix) MAKE_UNIQUE_NAME2(prefix)

void MAKE_UNIQUE_FUNCTION_NAME2(foo)(void)
{
	printf("Hello world\n");
}

// int main(int argc, char *argv[])  // alternative prototype
int main()
{
    printf("Autogenerate unique variable names containing the line number "
           "in them.\n\n");

    uint64_t MAKE_UNIQUE_VARIABLE_NAME1(aabb) = 0; 
    uint64_t MAKE_UNIQUE_VARIABLE_NAME1(aabb) = 0; 
    uint64_t MAKE_UNIQUE_VARIABLE_NAME1(aabb) = 0;

    uint64_t MAKE_UNIQUE_VARIABLE_NAME2(ccdd) = 0;
    uint64_t MAKE_UNIQUE_VARIABLE_NAME2(ccdd) = 0;
    uint64_t MAKE_UNIQUE_VARIABLE_NAME2(ccdd) = 0;

    // Uncomment this to suppress the errors.
	(void)aabb_32;
	(void)aabb_33;
	(void)aabb_31;

	(void)ccdd_1;
	(void)ccdd_2;
	(void)ccdd_3;

	foo_0();
	printf("%lu, %lu\n", aabb_31, ccdd_2);

    return 0;
}
