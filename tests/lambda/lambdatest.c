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
  usize (*g1)(usize, usize, usize);
  usize (*g2)(usize);

  if ((mem = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == MAP_FAILED) err(-1, "mmap");
  p = pool_new(mem, 1024, LAMBDA_SIZE(4));
  if (!(g1 = pool_alloc(p)) || !(g2 = pool_alloc(p))) errx(-1, "pool_alloc");

  if (!bind(g1, f1, 3, PH(1), PH(2), PH(0))) err(-1, "bind");
  printf("2 * 1 + 25: %lu\n", g1(25, 2, 1));
  if (!bind(g2, f2, 4, PH(0), PH(0), PH(0), PH(0))) err(-1, "bind");
  printf("g: %lx\n", g2(0xBE));
  mprotect(mem, 1024, PROT_READ | PROT_WRITE);
  pool_free(p, g1);
  pool_free(p, g2);
  return 0;
}
