//
// Created by 李一航 on 2018/9/24.
//

#ifndef NO2_JJ_H
#define NO2_JJ_H

#include <iostream> // for ecrr
#include <new>      // for placement new
#include <cstddef>  // for ptrdiff_t,size_t
#include <cstdlib>  // for exit()
#include <climits>  // for UINT_MAX

/*
 * 空间配置器
 * allocator
 */

namespace JJ
{
    using namespace std;
    // 配置空间
    template <typename T>
    inline T* _allocate( ptrdiff_t size, T* )
    {
        set_new_handler(0);
        T* temp = (T*)(::operator new( (size_t)( size * sizeof(T) ) ) );
        if( temp == nullptr )
        {
            cerr << "out of memory" << endl;
            exit(1);
        }
        return temp;
    }
    // 释放空间
    template <typename T>
    inline void _deallocate( T* buffer )
    {
        ::operator delete( buffer );
    }

    // 显式构造
    template <typename T1, typename T2>
    inline void _construct( T1* p, const T2& value )
    {
        new(p) T1( value );
    }

    // 显式析构
    template <typename T>
    inline void _destroy( T* ptr )
    {
        ptr->~T();
    }

    // 类
    template <typename T>
    class allocator
    {
    public:
        typedef T           value_type;
        typedef T*          pointer;
        typedef const T*    const_pointer;
        typedef T&          reference;
        typedef const T&    const_reference;
        typedef size_t      size_type;
        typedef ptrdiff_t   difference_type;

        // 重绑定
        template <typename U>
        struct rebind
        {
            typedef allocator<U> other;
        };

        pointer allocate( size_type n, const void* hint = 0 )
        {
            cout << "use allocate" << endl;
            return _allocate( (difference_type)n, (pointer)0 );
        }

        void deallocate( pointer p, size_type n )
        {
            cout << "use deallocate" << endl;
            _deallocate( p );
        }

        void construct( pointer p, const_reference value )
        {
            cout << "use construct" << endl;
            _construct( p, value );
        }

        void destroy( pointer p )
        {
            cout << "use destroy" << endl;
            _destroy( p );
        }

        pointer address( reference x )
        {
            return (pointer) &x;
        }

        const_pointer const_address( const_reference x )
        {
            return (const_pointer) &x;
        }

        size_type max_size() const
        {
            return size_type( UINT_MAX / sizeof(T) );
        }
    };

}




#endif //NO2_JJ_H
