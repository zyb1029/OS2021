#include <common.h>
#define PAGE_SIZE      4096
#define MAX_CPU        8
#define MAX_DATA_SIZE  8
#define MAX_PAGE       3000
#define LUCK_NUMBER    10291223
#define MAX_LIST       50000
#define MAX_BIG_SLAB   2048

typedef struct{
	int flag;	
}spinlock_t;


static int DataSize[MAX_DATA_SIZE] = {8, 16, 32, 64, 128, 512, 1024, 2048};
static int power[MAX_DATA_SIZE]    = {7, 15, 31, 63, 127, 15, 7, 7};

struct page_t{
	spinlock_t *lock;
	struct page_t *next;
	int block_size;
	int magic;
	int belong;
	int remain;
};

struct ptr_t{
  uintptr_t slot[256];
};

struct ptr_t *_ptr[MAX_PAGE]; 

struct page_t *page_table[MAX_CPU][MAX_DATA_SIZE];

spinlock_t lock[MAX_CPU];

void spinlock(spinlock_t *lk) {
	while(atomic_xchg(&lk -> flag, 1));	
}

void spinunlock(spinlock_t *lk) {
	atomic_xchg(&lk -> flag, 0);
}

int judge_size(size_t size) {
	for (int i = 0; i < MAX_DATA_SIZE; i++)
		if (size <= DataSize[i]) return i;
	if (size <= 4096) return MAX_DATA_SIZE;
	else assert(0);
}

void* deal_slab(int id, int kd) {
	struct page_t *now;
	now = page_table[id][kd];
	while (now != NULL && now -> remain == 0) now = now ->next;
	assert(now != NULL);
	assert(now -> remain != 0);
    return (void *)_ptr[now -> belong] -> slot[--now -> remain];	
}

void deal_slab_free(struct page_t *now, void *ptr) {
	assert(now -> magic == LUCK_NUMBER);
	_ptr[now -> belong] -> slot[now -> remain ++] = (uintptr_t)ptr;
}

struct node{
	int val_next[MAX_LIST];
	uintptr_t val_l[MAX_LIST], val_r[MAX_LIST];
	int val_valid[MAX_LIST], sum1;
    
	int delete_next[MAX_LIST];
	uintptr_t delete_l[MAX_LIST], delete_r[MAX_LIST];
	int delete_valid[MAX_LIST], sum2;
}*List;

uintptr_t BigSlab[MAX_BIG_SLAB];
static int BigSlab_Size = 0;
uintptr_t lSlab, rSlab;

void deal_SlowSlab_free(void *ptr) {
	BigSlab[BigSlab_Size++] = (uintptr_t)ptr;	
}

void *SlowSlab_path() {
	if (BigSlab_Size > 0) return (void *)BigSlab[--BigSlab_Size];
	else assert(0);
}

spinlock_t BigLock_Slow;
spinlock_t BigLock_Slab;


static void *kalloc(size_t size) {
  if ((size >> 20) > 16) return NULL;
  int id = cpu_current();
  int kd = judge_size(size);
  void *space;
  if (kd < MAX_DATA_SIZE) {
	spinlock(&lock[id]);
	space = deal_slab(id, kd);
	spinunlock(&lock[id]);	  
	return space;
  }
  else if(kd == MAX_DATA_SIZE) {
	spinlock(&BigLock_Slab);
	space = SlowSlab_path();
	spinunlock(&BigLock_Slab);
	return space;
  }
  assert(0);
}

int judge_free(void *ptr) {
  struct page_t *now = (struct page_t *) ((uintptr_t) ptr & (~(PAGE_SIZE - 1)));	
  if (now -> magic == LUCK_NUMBER) return 1;
  else if ((uintptr_t)ptr >= lSlab && (uintptr_t)ptr < rSlab) return 2;
  else assert(0);
}

static void kfree(void *ptr) {
  int kd = judge_free(ptr);
  if (kd == 1) {  
	struct page_t *now = (struct page_t *) ((uintptr_t)ptr & (~(PAGE_SIZE - 1)));
	spinlock(now->lock);
	deal_slab_free(now, ptr);
	spinunlock(now->lock);
  }
  else if (kd == 2) {
	  spinlock(&BigLock_Slab);
	  deal_SlowSlab_free(ptr);
	  spinunlock(&BigLock_Slab);
  }
  else assert(0);
}


int pmax(int a, int b) {
	return a > b ? a : b;
}

static int cnt = 0;

struct page_t* alloc_page(int cpu_id, int memory_size, int kd) {
	if (kd == 1) {
		_ptr[cnt] = (struct ptr_t *)heap.start;
		heap.start = (void *)ROUNDUP(heap.start + PAGE_SIZE, PAGE_SIZE);	
		struct page_t *page = (struct page_t *)heap.start;
		page -> lock        = &lock[cpu_id];
		page -> next        = NULL;
		page -> block_size  = DataSize[memory_size];
		page -> belong      = cnt++;
		page -> magic       = LUCK_NUMBER; 
		page -> remain      = 0;
		for (uintptr_t k = (uintptr_t)heap.start + pmax(128, DataSize[memory_size]); 
					   k != (uintptr_t)heap.start + PAGE_SIZE;
					   k += DataSize[memory_size]) {
			_ptr[page -> belong] -> slot[page -> remain] = k;	
			page -> remain = page -> remain + 1;
	}
	heap.start = (void *)ROUNDUP(heap.start + PAGE_SIZE, PAGE_SIZE);	
	return page;
	}
	else if (kd == 2) {
		void * tep = heap.start;	
		heap.start = (void *)ROUNDUP(heap.start + PAGE_SIZE, PAGE_SIZE);	
		return (struct page_t *)tep;
	}
    else assert(0);
}

static void pmm_init() {
  assert(sizeof(DataSize) / sizeof(int) == MAX_DATA_SIZE);
  int tep = 0;
  for (int i = 0; i < MAX_DATA_SIZE; i++) tep += power[i];
  assert(tep <= MAX_PAGE);

  heap.start = (void *)ROUNDUP(heap.start, PAGE_SIZE);
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);

  BigLock_Slab.flag = 0;
  BigLock_Slow.flag = 0;
  
  int tot = cpu_count();
  for (int i = 0; i < tot; i++) {
	  lock[i].flag = 0;
	  for (int j = 0; j < MAX_DATA_SIZE; j++) {
		  page_table[i][j] = alloc_page(i, j, 1);
	  }
  }
  for (int i  = 0; i < tot; i++) {
	for (int j = 0; j < MAX_DATA_SIZE; j++) {
		struct page_t *now = page_table[i][j];
		for (int k = 0; k < power[j]; k++) {
			now -> next = alloc_page(i, j, 1);
			now = now -> next;	
		}		
	}	    
  }
  lSlab = (uintptr_t)heap.start;
  for (int i = 0; i < MAX_BIG_SLAB; i++)
	BigSlab[BigSlab_Size++] = (uintptr_t)alloc_page(0, 0, 2);
  rSlab = (uintptr_t)heap.start;
  printf("%d\n", sizeof(struct node));
  printf("Got %d MiB heap: [%p, %p)\n", (heap.end-heap.start) >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
