#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/memfd.h>

static void
__attribute__ ((noinline))
print_int (int v)
{
  printf ("%d\n", v);
}

int
main (void)
{
#ifndef __NR_memfd_create
  char tmpfname[] = "/home/drepper/.execmemXXXXXX";
  int fd = mkstemp (tmpfname);
  unlink (tmpfname);
#else
  int fd = syscall(__NR_memfd_create, "test", MFD_CLOEXEC);
#endif
  ftruncate (fd, 1000);
  char *writep = mmap (NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  char *execp = mmap (NULL, 4096, PROT_READ|PROT_EXEC, MAP_SHARED, fd, 0);

#if __x86_64__
  writep[0] = '\x48';
  writep[1] = '\x83';
  writep[2] = '\xec';
  writep[3] = '\x08';
  writep[4] = '\xb8';
  *(int *) (writep + 5) = __NR_getppid;
  writep[9] = '\x0f';
  writep[10] = '\x05';
  writep[11] = '\x89';
  writep[12] = '\xc7';
  writep[13] = '\x48';
  writep[14] = '\xb8';
  *(char **) (writep + 15) = (char *) &print_int;
  writep[23] = '\xff';
  writep[24] = '\xd0';
  writep[25] = '\x48';
  writep[26] = '\x83';
  writep[27] = '\xc4';
  writep[28] = '\x08';
  writep[29] = '\xc3';
#elif __i386__
  writep[0] = '\xb8';
  *(int *) (writep + 1) = __NR_getppid;
  writep[5] = '\xcd';
  writep[6] = '\x80';
  writep[7] = '\x50';
  writep[8] = '\xe8';
  *(int *) (writep + 9) = (char *) &print_int - (execp + 13);
  writep[13] = '\x58';
  writep[14] = '\xc3';
#else
# error "architecture not supported"
#endif

  fputs ("asm code: ", stdout);
  asm volatile ("call *%0" : : "r" (execp) : "ax", "cx", "dx");

  fputs ("getppid: ", stdout);
  print_int (getppid ());

  munmap (writep, 4096);
  munmap (execp, 4096);
  close (fd);
  return 0;
}
