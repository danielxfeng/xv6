void vmprint(pagetable_t pagetable);
pte_t * walk(pagetable_t pagetable, uint64 va, int alloc);