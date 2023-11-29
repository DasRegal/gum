#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define RUNIT_ATTACH_COUNT          10
#define RUNIT_CHECK_FIRST_FAILURE   ru_count_failure != 1 ? : printf("\nFailures:\n\n")
#define RUNIT_ERROR_TEXT            printf("%d) %s\n\tError %s(%d): ", ru_count_failure, s, __FILE__, __LINE__)

#define describe(text)              str[namespace] = text;
#define context(text)               describe(text)
#define it(text)                    describe(text) ru_is_it = 1;

#define to_eq(x)                    == x ? \
                                    ru_count_pass++ : \
                                    (    ru_count_failure++, \
                                         RUNIT_CHECK_FIRST_FAILURE, \
                                         RUNIT_ERROR_TEXT, \
                                         printf("Expected to equal to %d\n", x) \
                                    ); \
                                    ru_test++;

#define not_to_eq(x)                != x ? \
                                    ru_count_pass++ : \
                                    (   ru_count_failure++, \
                                        RUNIT_CHECK_FIRST_FAILURE, \
                                        RUNIT_ERROR_TEXT, \
                                        printf("Expected to not to equal to %d\n", x) \
                                    ); \
                                    ru_test++;

#define arr_to_eq(_a, _b, _s)       for (int i = 0; i < _s; i++) \
                                    { \
                                        if (_a[i] != _b[i]) \
                                        { \
                                            ru_count_failure++; \
                                            RUNIT_CHECK_FIRST_FAILURE; \
                                            RUNIT_ERROR_TEXT; \
                                            printf("Elemets with %d index not equal (0x%x != 0x%x)\n", i, _a[i], _b[i]); \
                                        } \
                                    } \
                                    ru_test++;

#define assert_false                (   ru_count_failure++, \
                                        RUNIT_CHECK_FIRST_FAILURE, \
                                        RUNIT_ERROR_TEXT, \
                                        printf("Failure stub\n") \
                                    ); \
                                    ru_test++;

#define do                          { \
                                        if (ru_func != NULL && ru_is_it) ru_func(); \
                                        sprintf(s, "%s", str[0]); \
                                        for (int i = 1; i <= namespace; i++) sprintf(s, "%s\n\t%s", s, str[i]); \
                                        namespace++; \
                                        if (namespace > RUNIT_ATTACH_COUNT) \
                                        { \
                                            printf("\nError (%d): There can only be %d levels attachment it()do...end\n\n", __LINE__, RUNIT_ATTACH_COUNT); \
                                            exit(1); \
                                        }
#define end                         } \
                                    sprintf(s, ""); \
                                    namespace--; \
                                    if (ru_is_it) ru_is_it = 0;

#define source(_str)

#define before_each  void ru_func()

__attribute__((weak)) void ru_func();

unsigned int    ru_test = 0;
unsigned int    ru_count_failure = 0;
unsigned int    ru_count_pass = 0;
char            *str[RUNIT_ATTACH_COUNT];
char            s[1024];
unsigned int    namespace = 0;
char            ru_is_it = 0;

void main(void)
{
    #include "includes"
    printf("\n%d examples, %d failures\n", ru_test, ru_count_failure);

}
