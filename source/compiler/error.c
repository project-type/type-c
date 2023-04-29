//
// Created by praisethemoon on 29.04.23.
//

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include "error.h"

void typec_assert(int cond, const char * rawcond, const char* func_name, int line, const char * fmt, ...) {
    if (cond)
        return;
    char temp[1024];
    va_list vl;
    va_start(vl, fmt);
    vsprintf(temp, fmt, vl);
    va_end(vl);
    fprintf(stdout, "Fatal error, assertion failed: `%s` in function `%s`, line %d \n", rawcond, func_name, line);
    fprintf(stdout, "%s", temp);
    fprintf(stdout, "\n");
    assert(cond);
}