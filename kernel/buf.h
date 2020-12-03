struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;    // 缓存数据块
  uint blockno; // 缓存数据块
  struct sleeplock lock;
  uint refcnt; // 引用计数
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};

