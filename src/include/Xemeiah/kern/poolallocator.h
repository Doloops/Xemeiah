#ifndef __XEM_KERN_POOLALLOCATOR_H
#define __XEM_KERN_POOLALLOCATOR_H

#include <Xemeiah/trace.h>

#define Log_PoolAllocator Debug

#include <list>
#include <cstddef>

namespace Xem
{
  /**
   * High-performance (non-thread-safe) Pool-based allocator
   * All provided pointers are freed when the PoolAllocator is freed
   */
  template<typename _Tp, int poolSz> class PoolAllocator;

  template<typename _Tp, int rawPoolSz=512> class PoolAllocator
  {
  public:
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
    typedef _Tp*       pointer;
    typedef const _Tp* const_pointer;
    typedef _Tp&       reference;
    typedef const _Tp& const_reference;
    typedef _Tp        value_type;

    class RawPool
    {
    public:
      static const size_t poolSz = rawPoolSz;
      size_t offset;
      RawPool* nextPool;
      char buffer[poolSz];

      RawPool ()
      {
        offset = 0;
        nextPool = NULL;          
      }
      
      ~RawPool () {}
    };

    class CurrentRawPool
    {
    public:
      RawPool* current;
      CurrentRawPool() { current = NULL; }
      ~CurrentRawPool() {}
    };

    RawPool headPool;
    CurrentRawPool currentPool;

    template<typename _Tp1> struct rebind
    { typedef PoolAllocator<_Tp1> other; };

    inline void createNewPool()
    {
       RawPool* newPool = new RawPool (); // (RawPool*) malloc ( sizeof ( RawPool) );
       if ( currentPool.current != &headPool )
         newPool->nextPool = currentPool.current;
       currentPool.current = newPool;
       Log_PoolAllocator ( "New pool at %p\n", currentPool.current );
    }

    inline _Tp* alloc ( size_t sz )
     {
#if PARANOID
       AssertBug ( sz <= RawPool::poolSz, "Asked too much ! sz=0x%lx, poolSz=0x%lx !\n", (unsigned long) sz, (unsigned long) RawPool::poolSz );
#endif

       if ( ! currentPool.current )
         {
           Log_PoolAllocator ( "this=%p, initializing currentPool.current to headPool=%p\n", this, &headPool );
           currentPool.current = &headPool;
         }

       Log_PoolAllocator ( "this=%p, Allocate size %lx, headPool=%p(offset=%lx), currentPool=%p(offset=%lx)\n",
          this, (unsigned long) sz, &headPool, (unsigned long) headPool.offset,
          currentPool.current, (unsigned long) currentPool.current->offset );
          
       AssertBug ( currentPool.current->offset <= RawPool::poolSz, 
          "Invalid pool offset=%lx > max size %lx\n",
          (unsigned long) currentPool.current->offset, (unsigned long) RawPool::poolSz );

       if ( currentPool.current->offset + sz > RawPool::poolSz )
         {
           AssertBug ( sz <= RawPool::poolSz, "Asked too much ! sz=0x%lx, poolSz=0x%lx !\n", 
              (unsigned long) sz, (unsigned long) RawPool::poolSz );
           createNewPool ();
         }

       AssertBug ( currentPool.current->offset + sz <= RawPool::poolSz, 
          "Pool too small ! current offset=%lx, sz=%lx, poolSz=%lx\n",
          (unsigned long) currentPool.current->offset, 
          (unsigned long) sz, (unsigned long) RawPool::poolSz );

       void* ptr = &(currentPool.current->buffer[currentPool.current->offset]);
       currentPool.current->offset += sz;

       Log_PoolAllocator ( "this=%p, Alloccated size=%lx, ptr=%p, from pool=%p (offset=%lx)\n",
            this, (unsigned long) sz, ptr, currentPool.current, (unsigned long) currentPool.current->offset );
       return (_Tp*) ptr;
     }

     inline _Tp* allocate (size_type n, const void* hint = 0)
     {
       Log_PoolAllocator ( "this=%p, Allocate number %lx, sz=%lx, total=%lx\n",
        this, (unsigned long) n, (unsigned long) sizeof(_Tp), (unsigned long) n*sizeof(_Tp) );
       return alloc ( n * sizeof(_Tp) );
     }
     
     void  deallocate (pointer __p, size_type n)
     {
       Log_PoolAllocator ( "this=%p, Destroy at %p, st=%lx\n", this, __p, (unsigned long) n );
       __p->~_Tp();
     }
     
     void construct(pointer __p, const _Tp& __val)
     {
       AssertBug ( __p, "Invalid pointer at p\n" );
       Log_PoolAllocator ( "Construct at %p (%p) - sz=%lx\n", __p, *__p, (unsigned long) sizeof(_Tp) );
       ::new((void*) __p) _Tp(__val);
     }
     
     void destroy ( pointer __p ) 
     {
       Log_PoolAllocator ( "Destroy at %p (%p)\n", __p, *__p );
       __p->~_Tp();
     }
     
     template<typename _Tp1>
     PoolAllocator ( const PoolAllocator<_Tp1>& _tp1 )
     {
       Log_PoolAllocator ( "New Pool Allocator at %p as copy from %p, headPool=%p, currentPool=%p\n", this, &_tp1, &headPool, currentPool.current );
     }
     
     PoolAllocator ()
     {
       Log_PoolAllocator ( "New Pool Allocator at %p, headPool=%p, currentPool=%p\n", this, &headPool, currentPool.current );
     }

     ~PoolAllocator ()
     {
        Log_PoolAllocator ( "Delete pool allocator at %p\n", this );

        RawPool* pool = currentPool.current;
        
        while ( pool && pool != &headPool )
          {
            RawPool* lastPool = pool->nextPool;
            Log_PoolAllocator ( "[NS:IP] Delete iPool %p\n", pool );
            delete ( pool );
            pool = lastPool;
          }

#if PARANOID
        headPool.offset = 0xdeadbeef;
        currentPool.current = (RawPool*) 0xdeadbeef; 
#endif        
     }
     
  };   

  template<typename _T1, typename _T2>
  inline bool
  operator!=(const PoolAllocator<_T1>&, const PoolAllocator<_T2>&)
   { return false; }  


};

#endif // __XEM_KERN_POOLALLOCATOR_H
