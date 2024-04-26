// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmems[NCPU];

void
kinit()
{
  char lockname[16];
  for (int i = 0; i < NCPU; ++i) {
    snprintf(lockname, sizeof(lockname), "kmem_CPU%d", i);
    initlock(&kmems[i].lock, lockname);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();
  int cpu_id = cpuid();
  pop_off();

  acquire(&kmems[cpu_id].lock);
  r->next = kmems[cpu_id].freelist;
  kmems[cpu_id].freelist = r;
  release(&kmems[cpu_id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int cpu_id = cpuid();
  pop_off();

  acquire(&kmems[cpu_id].lock);
  r = kmems[cpu_id].freelist;

  if (!r) {
    int max_pages = 1024;
    for (int i = 0; i < NCPU; i++) {
      if (i == cpu_id) {
        continue;
      }
      acquire(&kmems[i].lock);
      if (!kmems[i].freelist || !kmems[i].freelist->next) {
        release(&kmems[i].lock);
        continue;
      }
      struct run *stolen_start = kmems[i].freelist;
      struct run *stolen_end = kmems[i].freelist;
      while (max_pages > 0 && stolen_end->next) {
        stolen_end = stolen_end->next;
        max_pages--;
      }
      kmems[i].freelist = stolen_end->next;
      release(&kmems[i].lock);
      stolen_end->next = 0;
      kmems[cpu_id].freelist = stolen_start;
      r = kmems[cpu_id].freelist;
      break;
    }
  }

  if(r) {
    kmems[cpu_id].freelist = r->next;
  }
  release(&kmems[cpu_id].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
