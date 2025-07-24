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
  char  mov;
} arg_t;

#define LDR(x) ((arg_t){ x, 0 })
#define MOV(x) ((arg_t){ x, 1 })
#ifdef __aarch64__
#define LAMBDA_MAX(n) (12 * (n) + 8)
#define LAMBDA_SIZE(movs, ldrs, cycles) (4 * ((movs) + (cycles)) + 12 * (ldrs) + 8)
#define MAX_ARGS 8
#endif

// creates a new function g from a function f, number of arguments n, and
// variadic arguments specified using the LDR and MOV macros. Arguments using
// LDR will inserted at the corresponding argument index, whereas arguments
// using MOV will move the data stored at g's argument index to the
// corresponding destination index. The maximum required size is given by
// LAMBDA_MAX(n) and the exact size can be computed with
// LAMBDA_SIZE(movs, ldrs, cycles), where cycles is the total number of disjoint
// cycles in the mov operations. The maximum value for n is given by MAX_ARGS.
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
#define ldr(r, o) 0x58000000 + ((o) << 3) + (r)
#define mov(d, s) 0xAA0003E0 + ((s) << 16) + (d)
#define br(r)     0xD61F0000 + ((r) << 5)

enum { TO_MOVE, BEING_MOVED, MOVED };

// https://inria.hal.science/inria-00289709
static void move_one(uint32_t **p, int n, int i, int src[static n], const int dst[static n], char status[static n]) {
  if (src[i] != dst[i]) {
    status[i] = BEING_MOVED;
    for (int j = 0; j < n; j++)
      if (src[j] == dst[i])
        switch (status[j]) {
          case TO_MOVE: move_one(p, n, j, src, dst, status); break;
          case BEING_MOVED: *(*p)++ = mov(src[j] = 16, src[j]); break;
        }
    *(*p)++ = mov(dst[i], src[i]);
    status[i] = MOVED;
  }
}

usize (*lambda_bind(usize (*g)(), usize (*f)(), int n, ...))() {
  uint32_t *p = (uint32_t *)g;
  usize *d;
  va_list args;
  int n_ldr = 0, n_mov = 0, msrc[MAX_ARGS] = {}, mdst[MAX_ARGS] = {}, ldst[MAX_ARGS] = {};
  usize lsrc[MAX_ARGS] = {};
  char status[MAX_ARGS] = { 0 };

  va_start(args, n);
  for (int i = 0; i < n; i++) {
    arg_t arg = va_arg(args, arg_t);
    if (arg.mov) {
      msrc[n_mov] = arg.val;
      mdst[n_mov++] = i;
    }
    else {
      lsrc[n_ldr] = arg.val;
      ldst[n_ldr++] = i;
    }
  }
  va_end(args);

#if LAMBDA_USE_MPROTECT
  if (mprotect((void *)((usize)g & ~0xFFF), LAMBDA_SIZE(n), PROT_READ | PROT_WRITE)) return NULL;
#endif

  for (int i = 0; i < n_mov; i++)
    if (status[i] == TO_MOVE) move_one(&p, n_mov, i, msrc, mdst, status);

  d = (usize *)(p + (n - n_mov) + 2);
  for (int i = 0; i < n_ldr; i++) {
    *p = ldr(ldst[i], (usize)d - (usize)p);
    p++;
    *d++ = lsrc[i];
  }

  *p = ldr(16, (usize)d - (usize)p);
  *++p = br(16);
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
