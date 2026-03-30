#define CLOVE_IMPLEMENTATION
#include "clove-unit.h"

#define CLOVE_SUITE_NAME MySuite02

int setup_once_count = 0;
int teardown_once_count = 0;
int setup_count = 0;
int teardown_count = 0;

CLOVE_SUITE_SETUP_ONCE() {
    setup_once_count++;
}

CLOVE_SUITE_TEARDOWN_ONCE() {
    teardown_once_count++;
}

CLOVE_SUITE_SETUP() {
   setup_count++;
}

CLOVE_SUITE_TEARDOWN() {
    teardown_count++;
}

CLOVE_TEST(A_FirstTest) {
    CLOVE_INT_EQ(1, setup_once_count);
    CLOVE_INT_EQ(0, teardown_once_count);
    CLOVE_INT_EQ(1, setup_count);
    CLOVE_INT_EQ(0, teardown_count);
}

CLOVE_TEST(B_SecondTest) {
    CLOVE_INT_EQ(1, setup_once_count);
    CLOVE_INT_EQ(0, teardown_once_count);
    CLOVE_INT_EQ(2, setup_count);
    CLOVE_INT_EQ(1, teardown_count);
}

#undef CLOVE_SUITE_NAME
#define CLOVE_SUITE_NAME MySuite03


CLOVE_SUITE_SETUP_ONCE() {
    setup_once_count++;
}

CLOVE_SUITE_TEARDOWN_ONCE() {
    teardown_once_count++;
}

CLOVE_SUITE_SETUP() {
   setup_count++;
}

CLOVE_SUITE_TEARDOWN() {
    teardown_count++;
}

CLOVE_TEST(A_2_FirstTest) {
    CLOVE_INT_EQ(1, setup_once_count);
    CLOVE_INT_EQ(0, teardown_once_count);
    CLOVE_INT_EQ(1, setup_count);
    CLOVE_INT_EQ(0, teardown_count);
}

CLOVE_TEST(B_2_SecondTest) {
    CLOVE_INT_EQ(1, setup_once_count);
    CLOVE_INT_EQ(0, teardown_once_count);
    CLOVE_INT_EQ(2, setup_count);
    CLOVE_INT_EQ(1, teardown_count);
}

#undef CLOVE_SUITE_NAME
#define CLOVE_SUITE_NAME MySuite03

CLOVE_TEST(BooleanTest) {
    CLOVE_IS_TRUE(1);
}

CLOVE_TEST(CharTest) {
    CLOVE_CHAR_EQ('a', 'a');
    CLOVE_CHAR_NE('a', 'b');
}

CLOVE_TEST(IntTest) {
    CLOVE_INT_EQ(2-1, 1);
}

CLOVE_TEST(UIntTest) {
    unsigned int value = 1;
    CLOVE_UINT_EQ(value, 1u);
}

CLOVE_TEST(LongfamilyTest) {
    long l = 1;
    long long ll = 1;
    unsigned long ul = 1;
    unsigned long long ull = 1;
    
    CLOVE_LONG_EQ(l, 1L);
    CLOVE_LLONG_EQ(ll, 1LL);
    CLOVE_ULONG_EQ(ul, 1UL);
    CLOVE_ULLONG_EQ(ull, 1ULL);
}

CLOVE_TEST(FloatTest) {
    CLOVE_FLOAT_NE(1.0f, 1.0001f);
    CLOVE_FLOAT_EQ(1.0f, 1.000001f); //Two float are equals if their diff is <= 0.000001f 
}

CLOVE_TEST(DoubleTest) {
    CLOVE_DOUBLE_NE(1.0, 1.0001);
    CLOVE_DOUBLE_EQ(1.0, 1.000001); //Two double are equals if their diff is <= 0.000001f 
}

CLOVE_TEST(StringTest) {
    CLOVE_STRING_NE("123", "1234");
    CLOVE_STRING_EQ("123", "123");

    const char array[] = {'1', '2', '3', '\0'};
    CLOVE_STRING_EQ(array, "123");
}

CLOVE_TEST(PassTest) {
    CLOVE_PASS();
}

CLOVE_TEST(FailTest) {
    CLOVE_FAIL();
}

CLOVE_RUNNER()
