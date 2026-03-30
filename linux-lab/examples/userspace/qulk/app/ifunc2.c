#include <stdio.h>
#include <stdlib.h>

// User-defined flag to select the version of calculate()
int use_optimized = 1;

// Optimized implementation of the function
int calculate_optimized(int a, int b) {
    printf("Using optimized version\n");
    return a * b;  // Let's say the optimized version does multiplication
}

// Debug implementation of the function
int calculate_debug(int a, int b) {
    printf("Using debug version\n");
    return a + b;  // The debug version simply adds the two numbers
}

// Resolver function that selects the implementation based on the user-defined flag
int (*resolve_calculate(void))(int, int) {
	printf("Calling resolve by %d\n", use_optimized);
    if (use_optimized) {
        return calculate_optimized;
    } else {
        return calculate_debug;
    }
}

// Declare the calculate function using the 'ifunc' attribute
int calculate(int a, int b) __attribute__((ifunc("resolve_calculate")));

int main() {
    int a = 5, b = 10;

	/* The test code only works for one case.
	 * ifunc only bound at startup time. 
	 */
    // Case 1: Use debug version (default)
    printf("Result (debug): %d\n", calculate(a, b));

    // Case 2: Use optimized version
    use_optimized = 1;
    printf("Result (optimized): %d\n", calculate(a, b));

    return 0;
}

