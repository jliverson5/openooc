/*
Copyright (c) 2015,2016 Jeremy Iverson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/* assert */
#include <assert.h>

/* SIGSEGV */
#include <errno.h>

/* uintptr_t */
#include <inttypes.h>

/* __WORDSIZE */
#include <limits.h>

/* setjmp, longjmp */
#include <setjmp.h>

/* sigaction */
#include <signal.h>

/* printf */
#include <stdio.h>

/* NULL, EXIT_SUCCESS, EXIT_FAILURE */
#include <stdlib.h>

/* ucontext_t */
#include <ucontext.h>


/* Helper macro to suppress unused variable/parameter warnings. */
#define UNUSED(var) (void)(var)


/* The stack environment to return to via longjmp. */
static __thread jmp_buf ret_env;

/* Memory location which caused fault */
static __thread void * segv_addr;


/*******************************************************************************
 * Function to return to from SIGSEGV handler instead of back to the instruction
 * which raised the fault.
 ******************************************************************************/
static void
ooc_async(void)
{
  printf("received SIGSEGV at address: %p.\n", segv_addr);

  longjmp(ret_env, 1);
}


/*******************************************************************************
 *  SIGSEGV handler.
 ******************************************************************************/
static void
ooc_sigsegv(int const _sig, siginfo_t * const _si, void * const _uc)
{
  ucontext_t * uc;

  assert(SIGSEGV == _sig);

  uc = (ucontext_t*)_uc;

  segv_addr = _si->si_addr;
  
#if 64 == __WORDSIZE
  uc->uc_mcontext.gregs[REG_RIP] = (greg_t)&ooc_async;
#else
  uc->uc_mcontext.gregs[REG_EIP] = (greg_t)&ooc_async;
#endif

  UNUSED(_si);
}


#ifdef TEST
int
main(int argc, char * argv[])
{
  int ret;
  struct sigaction act;

  act.sa_sigaction = &ooc_sigsegv;
  act.sa_flags = SA_SIGINFO;
  ret = sigaction(SIGSEGV, &act, NULL);
  assert(!ret);

  ret = setjmp(ret_env);
  if (!ret) {
    *(int*)0x1 = 1; /* Raise a SIGSEGV. */
    return EXIT_FAILURE;
  }

  printf("setjmp returned with code: %d.\n", ret);

  return EXIT_SUCCESS;

  UNUSED(argc);
  UNUSED(argv);
}
#endif
