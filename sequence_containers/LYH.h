//
// Created by 李一航 on 2018/10/1.
//

#ifndef NO2_LYH_H
#define NO2_LYH_H

#if 0
# include <new>
# define __THROW_BAD_ALLOC throw bad_alloc
#elif !defined( __THROW_BAD_ALLOC )
# include <iostream>
# define __THROW_BAD_ALLOC std::cerr<< "out of memory" << std::endl;exit(1)
#endif

#include <cstddef>
#include <cstdlib>


namespace LYH
{
    // 构造和析构的基本工具
    template <class T1, class T2>
    inline void construct( T1* p, const T2& value )
    {
        new(p) T1(value);
    }

    // destroy第一个版本，接受一个指针
    template <class T>
    inline void destroy( T* p )
    {
        p->~T();
    }

    // destroy第二个版本，接受两个迭代器
    // 接口
    template <class ForwardIterator>
    inline void destroy( ForwardIterator first, ForwardIterator last )
    {
        __destroy( first, last, value_type( first ) );
    }

    // 判断元素的数值型别是否有trivial dtor
    template <class ForwardIterator, class T>
    inline void __destroy( ForwardIterator first, ForwardIterator last, T* )
    {
        typedef typename __type_traits<T>::has_trivial_destructor trivial_destructor;
        __destroy_aux( first, last, trivial_destructor() );
    }

    // 如果有non-trivial dtor
    template <class ForwardIterator>
    inline void __destroy_aux( ForwardIterator first, ForwardIterator last, __false_type )
    {
        for( ; first < last; first++ )
            destroy( &*first );
    }

    // 如果有trivial dtor
    template <class ForwardIterator>
    inline void __destroy_aux( ForwardIterator first, ForwardIterator last, __true_type )
    {}  // 内置类型，什么都不做，把空间还回去即可


    // 统一接口
    template <class T, class Alloc>
    class simple_alloc
    {
    public:
        static T* allocate( size_t n )
            { return 0 == n? 0 : (T*)Alloc::allocate( n * sizeof(T) ); }
        static T* allocate( void )
            { return (T*) Alloc::allocate( sizeof(T) ); }
        static void deallocate( T* p, size_t n )
            { if( 0 != n ) Alloc::deallocate( p, n * sizeof(T) ); }
        static void deallocate( T* p )
            { Alloc::deallocate( p, sizeof(T) ); }
    };

    template <int inst>
    class __malloc_alloc_template
    {
    private:
        // 处理内存不足的情况
        // oom: out of memory
        static void* oom_malloc( size_t );
        static void* oom_realloc( void*, size_t );
        static void (* __malloc_alloc_oom_handler)();   // 函数指针

    public:
        static void* allocate( size_t n )
        {
            void* result = malloc(n);       // 第一级配置器直接使用malloc()
            if( 0 == result )
                result = oom_malloc(n);
            return result;
        }

        static void deallocate( void* p, size_t )
        {
            free(p);    // 第一级配置器直接使用free()
        }

        static void* reallocate( void* p, size_t /* old_sz */, size_t new_sz )
        {
            void* result = realloc( p, new_sz );        // 第一级配置器直接使用realloc
            if( 0 == result )
                result = oom_realloc( p, new_sz );
            return result;
        }

        // 指定自己的out-of-memory handler
        // 传进来的函数指针即为要设置的oom例程
        static void ( * set_malloc_handler( void(*f)() ) )()
        {
            // 声明一个返回类型为void，参数列表为空的函数指针
            void( * old )() = __malloc_alloc_oom_handler;
            __malloc_alloc_oom_handler = f;
            return old;
        }

    };

    // malloc_alloc out-of-memory handling
    template <int inst>
    void (* __malloc_alloc_template<inst>::__malloc_alloc_oom_handler )() = 0;

    template <int inst>
    void * __malloc_alloc_template<inst>::oom_malloc( size_t n )
    {
        // 声明函数指针
        void ( * my_malloc_handler )();
        void *result;

        for(;;)         // 不断尝试释放、配置、再释放、再配置
        {
            my_malloc_handler = __malloc_alloc_oom_handler;
            if( 0 == my_malloc_handler ){ __THROW_BAD_ALLOC; }
            (*my_malloc_handler)();     // 调用处理例程，企图释放内存
            result = malloc(n);         // 再次尝试配置内存
            if( result ) return result;
        }
    }

    template <int inst>
    void * __malloc_alloc_template<inst>::oom_realloc( void * p, size_t n )
    {
        // 声明函数指针
        void ( * my_malloc_handler )();
        void *result;

        for(;;)         // 不断尝试释放、配置、再释放、再配置
        {
            my_malloc_handler = __malloc_alloc_oom_handler;
            if( 0 == my_malloc_handler ){ __THROW_BAD_ALLOC; }
            (*my_malloc_handler)();         // 调用处理例程，企图释放内存
            result = realloc( p, n );       // 再次尝试配置内存
            if( result ) return result;
        }
    }

    // 以下直接将参数inst指定为0
    typedef __malloc_alloc_template<0> malloc_alloc;

    enum { __ALIGN = 8 };       // 小型区块的上调边界（即 基数）
    enum { __MAX_BYTES = 128 };     // 小型区块的上限
    enum { __NFREELISTS = __MAX_BYTES / __ALIGN };  // free-lists 个数

    template <bool threads, int inst>
    class __default_alloc_template
    {
    private:
        // 将输入的需求内存变为8的倍数
        static size_t ROUND_UP( size_t bytes )
        {
            return ( (bytes) + __ALIGN - 1 ) & ~( __ALIGN - 1 );
        }

        // free-list节点结构
        union obj
        {
            union obj* free_list_link;
            char client_data[1];
        };

        // 16个free-lists
        // 用来存放不同大小区块free list的目前可用头节点
        static obj* volatile free_list[__NFREELISTS];

        // 根据需求的区块大小，决定使用第几号free-list，n从0算起
        static size_t FREELIST_INDEX( size_t bytes )
        {
            return ( ( ( bytes + __ALIGN - 1 ) /  __ALIGN ) - 1  );
        }

        // 返回一个大小为n的区块，并可能将大小为n的其他区块加入到free list
        static void* refill( size_t n );

        // 从heap中配置一大块空间，可容纳nobjs个大小为size的区块
        // 如果配置nobjs个区块力不能及，nobjs会减小
        static char* chunk_alloc( size_t size, int &nobjs );

        static char* start_free;        // heap起始位置，只在chunk_alloc中变化
        static char* end_free;          // heap结束位置，只在chunk_alloc中变化
        static size_t heap_size;

    public:
        // 配置空间
        // n must > 0
        static void* allocate( size_t n )
        {
            obj* volatile * my_free_list;   // 二级指针，指向了free list数组的某一元素，free list数组的元素是指针
                                            // 目的是可以直接使用该指针维护free list数组
            obj* result;
            // 如果大于128就调用一级配置器
            if( n > (size_t) __MAX_BYTES )
                return ( malloc_alloc::allocate(n) );
            // 在16个free lists中寻找适当的一个头节点
            my_free_list = free_list + FREELIST_INDEX(n);
            result = my_free_list;
            if( nullptr == result )
            {
                // 没找到可用的free list，准备重新填充
                void *r = refill( ROUND_UP( n ) );
                return r;
            }
            // 维护free lists
            // 相当于链表指针后移
            *my_free_list = result->free_list_link;

            return result;
        }

        // 释放空间
        static void deallocate( void* p, size_t n )
        {
            obj* q = (obj*) p;
            obj* volatile * my_free_list;

            // 大于128就调用一级配置器
            if( n > (size_t) __MAX_BYTES )
            {
                malloc_alloc::deallocate( p, n );
                return;
            }
            // 寻找对应的free list
            my_free_list = free_list + FREELIST_INDEX( n );

            // 维护free lists
            // 链表的插入操作，插在free lists当前可用节点的前面
            q->free_list_link = *my_free_list;
            *my_free_list = q;
        }
    };

    // 初值设定
    template <bool threads, int inst>
    char* __default_alloc_template<threads, inst>::start_free = nullptr;

    template <bool threads, int inst>
    char* __default_alloc_template<threads, inst>::end_free = nullptr;

    template <bool threads, int inst>
    size_t __default_alloc_template<threads, inst>::heap_size = 0;

    template <bool threads, int inst>
    typename __default_alloc_template<threads, inst>::obj* volatile
    __default_alloc_template<threads, inst>::free_list[__NFREELISTS] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    // 返回一个free list节点供客户端使用，并重新填充该free list（调用refill意味着原先的已经用完）
    // 这里的n已经处理为8的倍数
    template <bool threads, int inst>
    void* __default_alloc_template<threads, inst>::refill( size_t n )
    {
        int nobjs = 20;     // 缺省申请20个新节点
        char* chunk = chunk_alloc( n, nobjs );  // 这里nobjs为引用传递，因为可能存在不够供给20个节点的空间，可修改nobjs的值
        obj* volatile * my_free_list;
        obj* result;
        obj* current_obj, *next_obj;
        int i;

        // 只够分一个，则将分到的给到客户端
        if( 1 == nobjs ) return chunk;

        // 分到多个区块
        result = (obj*) chunk;      // 这一块留给客户端

        // 重新填充free list（进入该函数，证明当前free list已经为空，需要重新建立链表）
        my_free_list = free_list + FREELIST_INDEX( n );
        *my_free_list = next_obj = (obj*) ( chunk + n );    // 第一个给了，这里计算偏移，维护free list第一个节点，并初始化next_obj
        for( i = 1; ; ++i )
        {
            current_obj = next_obj;
            next_obj = (obj*)( (char*)next_obj + n );
            if( nobjs - 1 == i )
            {
                current_obj->free_list_link = 0;
                break;
            }
            else
            {
                current_obj->free_list_link = next_obj;
            }
        }
        return result;
    }

    // 内存池操作
    // size已适当上调至8的倍数
    template <bool threads, int inst>
    char* __default_alloc_template<threads, inst>::chunk_alloc( size_t size, int &nobjs )
    {
        char* result;
        size_t total_bytes = size * nobjs;              // 总共需要申请的空间
        size_t bytes_left = end_free - start_free;      // 内存池剩余空间

        if( bytes_left>= total_bytes )
        {
            // 内存池剩余空间满足需求
            result = start_free;
            start_free += total_bytes;
            return result;
        }
        else if( bytes_left >= size )
        {
            // 剩余空间不能满足需求，但可以满足一个及以上的区块
            nobjs = int( bytes_left / size );
            total_bytes = nobjs * size;
            result = start_free;
            start_free += total_bytes;
            return result;
        }
        else
        {
            // 一个都不够
            // 压榨——把残存的都分配出去
            if( bytes_left > 0 )
            {
                // 寻找适当的free list
                obj* volatile * my_free_list = free_list + FREELIST_INDEX( bytes_left );
                // 添加进当前的free list
                ( (obj*)start_free )->free_list_link = *my_free_list;
                *my_free_list = (obj*)start_free;
            }

            // 配置heap空间，补充内存池
            size_t bytes_to_get = 2 * total_bytes + ROUND_UP( heap_size >> 4 );
            start_free = (char*)malloc( bytes_to_get );
            if( nullptr == start_free )
            {
                // heap空间不足，分配失败
                // 策略：从尚有未用区域，且区块够大的free list中释放内存至内存池中
                obj * volatile * my_free_list, *p;
                for( int i = size; i <= __MAX_BYTES; i += __ALIGN )
                {
                    my_free_list = free_list + FREELIST_INDEX( size );
                    p = *my_free_list;
                    if( 0 != p )
                    {
                        // free list内尚有未用区域，调整以释放
                        *my_free_list = p->free_list_link;
                        start_free = (char*) p;
                        end_free = start_free + i;
                        // 递归调用自己，修正nobjs
                        return ( chunk_alloc( size, nobjs ) );
                    }
                }
                end_free = nullptr;       // 山穷水尽，到处没有内存可用了
                // 调用第一级配置器，看oom机制能否找出内存
                start_free = (char*)malloc_alloc::allocate( bytes_to_get );
                // 这里或抛出异常，或有内存可用
                heap_size += bytes_to_get;
                end_free = start_free + bytes_to_get;
                // 递归调用自己，修正nobjs
                return ( chunk_alloc( size, nobjs ) );
            }
        }
    }

    typedef __default_alloc_template<false,0> alloc;

    template <class ForwardIterator, class Size, class T>
    inline ForwardIterator uninitialized_fill_n( ForwardIterator first,
                                                 Size n, const T& x )
    {
        return __uninitialized_fill_n( first, n, x, value_type( first ) );
    }
    // 萃取出迭代器first的value type，然后判断该类型是否为POD
    template <class ForwardIterator, class Size, class T, class T1>
    inline ForwardIterator __uninitialized_fill_n( ForwardIterator first, Size n,
                                                   const T& x, T1* )
    {
        typedef typename __type_traits<T1>::is_POD_type is_POD();
        return __uninitialized_fill_n_aux( first, n, x, is_POD );
    }

    // 如果copy construction 等同于 assignment
    // destructor是trivial，以下就有效
    // 如果是POD型别
    template <class ForwardIterator, class Size, class T>
    inline ForwardIterator __uninitialized_fill_n_aux( ForwardIterator first,
                                                       Size n, const T& x, __true_type )
    {
        return fill_n( first, n, x );       // 交由高阶函数执行
    }
    // 如果不是POD型别
    template <class ForwardIterator, class Size, class T>
    inline ForwardIterator __uninitialized_fill_n_aux( ForwardIterator first,
                                       Size n, const T& x, __false_type )
    {
        ForwardIterator cur = first;
        for( ; n > 0; --n, ++cur )
        {
            constrcut( &*cur, x );
        }
    }
};


#endif //NO2_LYH_H