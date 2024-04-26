// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BUCK_SIZ 21
#define BCACHE_HASH(dev, blk) (((dev << 27) | blk) % BUCK_SIZ)

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct spinlock item_locks[BUCK_SIZ];

  struct buf head[BUCK_SIZ];
} bcache;

void
binit(void)
{
  char lockname[16];
  for (int i = 0; i < BUCK_SIZ; i++) {
    snprintf(lockname, sizeof(lockname), "buff_Buck%d", i);
    initlock(&bcache.item_locks[i], lockname);
    bcache.head[i].next = 0;
  }
  initlock(&bcache.lock, "bcache");

  for(int i = 0; i < NBUF; i++){
    struct buf *b = &bcache.buf[i];
    snprintf(lockname, sizeof(lockname), "buff%d", i);
    initsleeplock(&b->lock, lockname);
    b->last_use = 0;
    b->refcnt = 0;
    b->next = bcache.head[0].next;
    bcache.head[0].next = b;
  }
}

struct buf* bfind_prelru(int* lru_bkt){
  struct buf* lru_res = 0;
  *lru_bkt = -1;
  struct buf* b;
  for (int i = 0; i < BUCK_SIZ; i++){
    acquire(&bcache.item_locks[i]);
    int found = 0;
    for (b = &bcache.head[i]; b->next; b = b->next) {
      if (!b->next->refcnt && (!lru_res || b->next->last_use < lru_res->next->last_use)) {
         lru_res = b;
         found = 1;
      }
    }
    if (!found) {
      release(&bcache.item_locks[i]);
    } else {
     if(*lru_bkt != -1) {
       release(&bcache.item_locks[*lru_bkt]);
     }
     *lru_bkt = i;
    }
  }
  return lru_res;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint key = BCACHE_HASH(dev, blockno);
  acquire(&bcache.item_locks[key]);

  // Is the block already cached?
  for(b = bcache.head[key].next; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.item_locks[key]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  release(&bcache.item_locks[key]);
  acquire(&bcache.lock);
  acquire(&bcache.item_locks[key]);
  for(b = bcache.head[key].next; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.item_locks[key]);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.item_locks[key]);
  int lru_bkt;
  struct buf* pre_lru = bfind_prelru(&lru_bkt);
  if (pre_lru == 0){
    panic("bget: no buffers");
  }
  struct buf* lru = pre_lru->next;
  pre_lru->next = lru->next;
  release(&bcache.item_locks[lru_bkt]);

  acquire(&bcache.item_locks[key]);
  lru->next = bcache.head[key].next;
  bcache.head[key].next = lru;

  lru->dev = dev,
  lru->blockno = blockno;
  lru->valid = 0,
  lru->refcnt = 1;

  release(&bcache.item_locks[key]);
  release(&bcache.lock);

  acquiresleep(&lru->lock);
  return lru;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint key = BCACHE_HASH(b->dev, b->blockno);
  acquire(&bcache.item_locks[key]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->last_use = ticks;
  }
  
  release(&bcache.item_locks[key]);
}

void
bpin(struct buf *b) {
  uint key = BCACHE_HASH(b->dev, b->blockno);
  acquire(&bcache.item_locks[key]);
  b->refcnt++;
  release(&bcache.item_locks[key]);
}

void
bunpin(struct buf *b) {
  uint key = BCACHE_HASH(b->dev, b->blockno);
  acquire(&bcache.item_locks[key]);
  b->refcnt--;
  release(&bcache.item_locks[key]);
}


