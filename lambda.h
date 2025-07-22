/* lambda.h - v0.1
 * Functional programming for C
 *
 * Copyright (c) 2025 Christopher Milan
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef LAMBDA_IMPLEMENTATION
#ifndef LAMBDA_DEFINITIONS
#define LAMBDA_DEFINITIONS

#define LAMBDA_USE_MPROTECT 0

#endif
#endif

#ifndef _LAMBDA_H
#define _LAMBDA_H

#include <stdint.h>

typedef uintptr_t usize;

typedef struct {
  usize val;
  char  ph;
} arg_t;

#define ARG(x) ((arg_t){ x, 0 })
#define PH(x) ((arg_t){ x, 1 })
#define LAMBDA_SIZE(n) (12 * (n) + 8)
#ifdef __aarch64__
#define MAX_ARGS 8
#endif

// creates a new function g from a function f, number of arguments n, and
// variadic arguments where the arguments to g are passed to f via the inputs
// marked as placeholders using the PH macro. The required size is given by
// LAMBDA_SIZE(n). The maximum value for n is given by MAX_ARGS.
usize (*lambda_bind(usize (*g)(), usize (*f)(), int n, ...))();

#endif
#ifdef LAMBDA_IMPLEMENTATION

#include <string.h>
#include <stdarg.h>
#ifdef LAMBDA_USE_MPROTECT
#include <sys/mman.h>
#endif

#ifdef __aarch64__

#define nop       0xD503201F
#define ldr(r, o) 0x58000000 + ((o) << 3) + r
#define mov(d, s) 0xAA0003E0 + (s << 16) + d
#define br(r)     0xD61F0000 + ((r) << 5)

usize (*lambda_bind(usize (*g)(), usize (*f)(), int n, ...))() {
  uint32_t *p = (uint32_t *)g;
  usize *d;
  va_list _args;
  arg_t args[MAX_ARGS];
  int src[MAX_ARGS];
  char done[MAX_ARGS] = { 0 };
  int n_ph = 0;

  va_start(_args, n);
  for (int i = 0; i < n; i++) {
    args[i] = va_arg(_args, arg_t);
    src[i] = args[i].ph ? args[i].val : i;
    if (args[i].ph) n_ph++;
  }
  va_end(_args);

#if LAMBDA_USE_MPROTECT
  if (mprotect((void *)((usize)g & ~0xFFF), LAMBDA_SIZE(n), PROT_READ | PROT_WRITE)) return NULL;
#endif

  for (int i = 0; i < n; i++) {
    int j;

    if (done[i] || src[i] == i) { done[i] = 1; continue; };
    if (done[src[i]]) { *p++ = mov(i, src[i]); done[i] = 1; continue; }
    *p++ = mov(16, i);
    for (j = i; src[j] != i && src[j] != j; j = src[j]) {
      *p++ = mov(j, src[j]);
      done[j] = 1;
    }
    *p++ = mov(j, 16);
    done[j] = 1;
  }

  d = (usize *)(p + (n - n_ph) + 2);
  for (int i = 0; i < n; i++) {
    if (!args[i].ph) {
      *p = ldr(i, (usize)d - (usize)p);
      p++;
      *d++ = args[i].val;
    }
  }

  *p = ldr(16, (usize)d - (usize)p);
  *(p+1) = br(16);
  *d = (usize)f;

#if LAMBDA_USE_MPROTECT
  if (mprotect((void *)((usize)g & ~0xFFF), LAMBDA_SIZE(n), PROT_READ | PROT_EXEC)) return NULL;
#endif

  return g;
}
#elif defined(__x86_64__)
#define u64(x) \
  (x >> 56) & 0xFF, (x >> 48) & 0xFF, (x >> 40) & 0xFF, (x >> 32) & 0xFF, \
  (x >> 24) & 0xFF, (x >> 16) & 0xFF, (x >>  8) & 0xFF, (x >>  0) & 0xFF

#define ri(r, i) 0x48, 0xBF + r, u64(i) /* mov reg, imm */
#define rr(r, s) 0x48, 0xBF + r, s      /* mov reg, reg */
#define j(addr)  0x48, 0xB8, u64(addr), /* mov rax, fn */ \
                 0xFF, 0xE0             /* jmp rax     */
#endif
#endif
