#include "lambda.h"
#include "pool.h"
#include <err.h>
#include <stdio.h>
#include <sys/mman.h>

usize f1(usize x, usize y, usize z) { return x * y + z; }
usize f2(usize a, usize b, usize c, usize d) { return (a << 24) | (b << 16) | (c << 8) | d; }

int main(void) {
  void *mem;
  pool_t p;
  usize (*g1)(usize, usize);
  usize (*g2)(usize);

  if ((mem = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == MAP_FAILED) err(-1, "mmap");
  p = pool_new(mem, 1024, LAMBDA_MAX(4));
  if (!(g1 = pool_alloc(p)) || !(g2 = pool_alloc(p))) errx(-1, "pool_alloc");

  if (!lambda_bind(g1, f1, 3, LDR(2), MOV(0), MOV(1))) err(-1, "lambda_bind");
  printf("2 * 1 + 25: %lu\n", g1(1, 25));
  if (!lambda_bind(g2, f2, 4, MOV(0), MOV(0), MOV(0), MOV(0))) err(-1, "lambda_bind");
  printf("g: %lx\n", g2(0xBE));
  mprotect(mem, 1024, PROT_READ | PROT_WRITE);
  pool_free(p, g1);
  pool_free(p, g2);
  return 0;
}
