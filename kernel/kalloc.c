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

// 空闲页
struct run {
  struct run *next;
};

static struct {
  struct spinlock lock; 
  struct run *freelist;
} kmems[NCPU];

// 初始化分配器
void
kinit()
{
  for (int i = 0; i < NCPU; i++)
    initlock(&kmems[i].lock, "kmem"); // 保存所有空闲页来初始化链表
  freerange(end, (void*)PHYSTOP); // 把空闲内存加到链表
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start); // 确保空闲内存是4K对齐的
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) // 按照页面大小切分
    kfree(p); // 将页面从头部插入到链表
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
// 释放内存空间
void
kfree(void *pa)
{
  struct run *r;
  int cpu_id;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE); // 初始化物理内存为1

  r = (struct run*)pa;

  push_off();
  cpu_id = cpuid();
  pop_off();

  acquire(&(kmems[cpu_id].lock)); // 获取锁
  r->next = kmems[cpu_id].freelist; // 将空闲页物理内存加到链表头
  kmems[cpu_id].freelist = r;
  release(&(kmems[cpu_id].lock)); // 释放锁
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
// 生成新内存空间
void *
kalloc(void)
{
  struct run *r;
  int cpu_id;

  push_off();
  cpu_id = cpuid();
  pop_off();

  acquire(&(kmems[cpu_id].lock)); // 获取锁
  r = kmems[cpu_id].freelist;
  if(r)
  {
    kmems[cpu_id].freelist = r->next; // 移除并返回空闲链表头的第一个元素
    release(&kmems[cpu_id].lock); // 释放锁
  }
  else
  {
    release(&kmems[cpu_id].lock); // 释放锁
    for (int i = 0; i < NCPU; i++)
    {
      if (i != cpu_id && kmems[i].lock.locked == 0)
      {
        acquire(&(kmems[i].lock));
        r = kmems[i].freelist; 
        if (r)
        {
          kmems[i].freelist = r->next;
          release(&kmems[i].lock);    
          break;
        }
        else
        {
          release(&kmems[i].lock);    
          continue;
        }
      }
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
