/* pool.h - v0.1
 * A memory pool allocator.
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

#ifndef _POOL_H
#define _POOL_H

#include <stdlib.h>

typedef struct pool * pool_t;

// configures a region of memory to work as a memory pool
pool_t pool_new(void *mem, int len, int chunk_sz);
// returns a pointer to a chunk of memory of size POOL_CHUNK_SIZE
void *pool_alloc(pool_t p);
// frees a chunk of memory previously allocated by pool_alloc
void pool_free(pool_t p, void *ptr);

#endif

#ifdef POOL_IMPLEMENTATION

typedef uintptr_t usize;
struct pool {
  int len, chunk_sz;
  void *wilderness;
  void *free;
  unsigned char data[];
};

pool_t pool_new(void *mem, int len, int chunk_sz) {
  pool_t p = mem;
  p->len = len;
  p->chunk_sz = chunk_sz;
  p->wilderness = &p->data;
  p->free = 0;
  return p;
}

void *pool_alloc(pool_t p) {
  void *ret;
  if (p->free) {
    ret = p->free;
    p->free = *(void **)p->free;
    return ret;
  }

  if ((usize)p->wilderness + p->chunk_sz > (usize)p + p->len) return 0;

  ret = p->wilderness;
  p->wilderness = (char *)p->wilderness + p->chunk_sz;
  return ret;
}

void pool_free(pool_t p, void *ptr) {
  if (ptr + p->chunk_sz == p->wilderness) p->wilderness -= p->chunk_sz;
  else {
    *(void **)ptr = p->free;
    p->free = ptr;
  }
  return;
}

#endif
