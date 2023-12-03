extern void *kalloc(unsigned long size);
extern void  kfree(void *addr);
extern void *memcpy(void *dest, const void *src, unsigned long n);
extern int printf(const char *format, ...);


#define fflush(...) 
// #define fprintf(fp, ...) printf(__VA_ARGS__)

static inline void
abort(void)
{
  while (1);
}

static inline void *
malloc(unsigned long size)
{
  return kalloc(size);
}

static inline void *
calloc(unsigned long nmemb, unsigned long size)
{
  return kalloc(nmemb*size);
}

static inline void
free(void *addr)
{
  kfree(addr);
}

static inline void *
realloc(void *ptr, unsigned long size)
{
  void *newp;

  newp = kalloc(size);
  if (!ptr)
    return newp;
  memcpy(newp, ptr, size);
  kfree(ptr);
  return newp;
}
  
  


