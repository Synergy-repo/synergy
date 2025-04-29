
#include <new>
#include <stdlib.h>

void *
operator new (size_t sz)
{
  void *ret = malloc (sz);

  if (!ret)
    throw std::bad_alloc ();

  return ret;
}

void *
operator new[] (size_t sz)
{
  return ::operator new (sz);
}

void
operator delete (void *p)
{
  free (p);
}

void
operator delete[] (void *p)
{
  ::operator delete (p);
}

void
operator delete (void *p, std::size_t)
{
  ::operator delete (p);
}

void
operator delete[] (void *p, std::size_t)
{
  ::operator delete (p);
}
