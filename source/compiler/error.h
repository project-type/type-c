//
// Created by praisethemoon on 29.04.23.
//

#ifndef TYPE_C_ERROR_H
#define TYPE_C_ERROR_H

#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
#define VM_ARCH "x64"
#elif defined(__arm64__) || defined(__aarch64__)
#define VM_ARCH "arm64"
#elif defined(__i386__) || defined(_M_IX86)
#define VM_ARCH "x86"
#else
    #define VM_ARCH "?"
#endif

#ifndef __FUNCTION_NAME__
#ifdef WIN32   //WINDOWS
#define __FUNCTION_NAME__   __FUNCTION__
#else          //*NIX
#define __FUNCTION_NAME__   __func__
#endif
#endif


#define TYPE_C_ASSERT_LOG

#if defined(TYPE_C_ASSERT_STANDARD)
#define ASSERT(c ,msg, ...) assert(c)
#elif defined (TYPE_C_ASSERT_LOG)
#define ASSERT(c, msg, ...) typec_assert(c, #c, __FUNCTION_NAME__, __LINE__ , msg, ##__VA_ARGS__)
#elif defined (TYPE_C_ASSERT_NONE)
#define ASSERT(c, msg, ...)
#else
#error Please define an assertion policy.
#endif


void typec_assert(int cond, const char * rawcond, const char* func_name, int line, const char * fmt, ...);

#endif //TYPE_C_ERROR_H
