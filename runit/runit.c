#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define RUNIT_CHECK_FIRST_FAILURE ru_count_failure != 1 ? : printf("\nFailures:\n\n")
#define RUNIT_CHECK_NAMESPACE namespace != 1 ? : desc_str = ""
#define describe(text) desc_str = text; it_str = "";
#define to_eq(x) == x ? \
                ru_count_pass++ : \
                (    ru_count_failure++, \
                     RUNIT_CHECK_FIRST_FAILURE, \
                     printf("%d) %s %s\n\tError %s(%d): Expected to equal to %d\n", ru_count_failure, desc_str, namespace == 1 ? "" : it_str, __FILE__, __LINE__, x) \
                ); \
                ru_test++;

#define not_to_eq(x) != x ? \
                ru_count_pass++ : \
                (   ru_count_failure++, \
                    RUNIT_CHECK_FIRST_FAILURE, \
                    printf("%d) %s %s\n\tError %s(%d): Expected to not to equal to %d\n", ru_count_failure, desc_str, namespace == 1 ? "" : it_str, __FILE__, __LINE__, x) \
                ); \
                ru_test++;

#define arr_to_eq(_a, _b, _s) for (int i = 0; i < _s; i++) \
                              { \
                                  if (_a[i] != _b[i]) \
                                  { \
                                      ru_count_failure++; \
                                      RUNIT_CHECK_FIRST_FAILURE; \
                                      printf("%d) %s %s\n\tError %s(%d): Elemets with %d index not equal (0x%x != 0x%x)\n", ru_count_failure, desc_str, namespace == 1 ? "" : it_str, __FILE__, __LINE__, i, _a[i], _b[i]); \
                                  } \
                              } \
                              ru_test++;

#define it(text) it_str = text; ru_it_count++;
#define do  { \
            namespace++; \
            if (namespace > 2) { printf("\nError (%d): There can only be one level attachment it()do...end\n\n", __LINE__); exit(1); }
#define end } \
            namespace--;

#define source(_str)

unsigned int ru_it_count = 0;
unsigned int ru_test = 0;
unsigned int ru_count_failure = 0;
unsigned int ru_count_pass = 0;
char * desc_str = "";
char * it_str = "";
unsigned int namespace = 0;

void main(void)
{
    #include "includes"
    printf("\n%d examples, %d failures\n", ru_test, ru_count_failure);
}
