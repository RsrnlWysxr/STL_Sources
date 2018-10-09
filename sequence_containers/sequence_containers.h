//
// Created by 李一航 on 2018/10/5.
//

#ifndef SEQUENCE_CONTAINERS_SEQUENCE_CONTAINERS_H
#define SEQUENCE_CONTAINERS_SEQUENCE_CONTAINERS_H

#include <iostream>
#include "LYH.h"

using namespace LYH;


template <class T, class Alloc = alloc>
class vector
{
public:
    // 内嵌型定义
    typedef T                    value_type;
    typedef value_type*          pointer;
    typedef value_type*          iterator;      // vector维护连续线性空间，迭代器为普通指针
    typedef value_type&          reference;
    typedef const value_type&    const_reference;
    typedef size_t               size_type;
    typedef ptrdiff_t            difference_type;

protected:
    typedef simple_alloc<value_type, Alloc> data_allocator;
    iterator start;             // 表示目前使用空间的头
    iterator finish;            // 表示目前使用空间的尾，永远指向一个空对象
    iterator end_of_storage;    // 表示目前可用空间的尾
    void insert_aux( iterator position, const_reference x )
    {
        if( finish != end_of_storage )  // 还有备用空间
        {
            // 在备用空间起始处构造一个元素，并以vector最后一个元素值为其初值
            construct( finish, *(finish - 1) );
            ++finish;
            T x_copy = x;
            std::copy_backward( position, finish-2, finish-1 );
            *position = x_copy;
        }
        else
        {
            // 没有备用空间了
            const size_type old_size = size();
            const size_type len = old_size != 0? 2 * old_size : 1;
            // 新开辟空间
            iterator new_start = data_allocator::allocate( len );
            iterator new_finish = new_start;
            try
            {
                new_finish = uninitialized_fill_copy( start, position, new_start );
                construct( new_finish, x );
                ++new_finish;
                // 将安插点的原内容也拷贝过来
                new_finish = uninitialized_fill_copy( position, finish, new_finish );
            }
            catch(...)
            {
                destroy( new_start, new_finish );
                data_allocator::deallocate( new_start, len );
                throw;
            }

            // 析构并释放原vector
            destroy( begin(), end() );
            deallocate();

            // 调整迭代器，指向新的vector
            start = new_start;
            finish = new_finish;
            end_of_storage = new_start + len;
        }
    }
    void deallocate()
    {
        if( start )
            data_allocator::deallocate( end_of_storage - start );
    }
    void fill_initialize( size_type n, const_reference value )
    {
        start = allocate_and_fill( n, value );
        finish = start + n;
        end_of_storage = finish;
    }
    // 配置空间并填满内容
    iterator allocate_and_fill( size_type n, const_reference x )
    {
        iterator result = data_allocator::allocate(n);
        uninitialized_fill_n( result, n, x );
        return result;
    }

public:
    iterator begin() { return start; }
    iterator end()   { return finish; }
    size_type size() const { return size_type( end() - begin() ); }
    size_type capacity() const
        { return size_type( end_of_storage - start ); }
    bool empty() { return begin() - end() == 0; }
    reference operator[]( size_type n )
        { return *(begin() + n); }

    // ctor
    vector() : start(0), finish(0), end_of_storage(0) {}
    vector( size_type n, const_reference value ) { fill_initialize( n, value ); }
    vector( int n, const_reference value ) { fill_initialize( n, value ); }
    vector( long n, const_reference value ) { fill_initialize( n, value ); }
    explicit vector( size_type n ) { fill_initialize( n, T() ); }       // T的默认构造函数

    // dtor
    ~vector()
    {
        destroy( start, finish );
        deallocate();
    }

    // 第一个元素
    reference front() { return *begin(); }
    // 最后一个元素
    reference back() { return *( end() - 1 ); }
    // 将元素插入至尾端
    void push_back( const_reference x )
    {
        if( finish != end_of_storage )  // 还有备用空间
        {
            construct( finish, x );
            finish++;
        }
        else
        {
            insert_aux( end(), x );
        }
    }
    // 将最尾端元素取出
    void pop_back()
    {
        --finish;
        destroy( finish );
    }
    // 清除某位置上的元素
    iterator erase( iterator position )
    {
        if( position + 1 != end() ) // position在有效区间内
            bcopy( position+1, finish, position );  // 后续元素往前移动
        --finish;
        destroy(finish);
        return position;
    }
    // 清楚[first,last)中的所有元素
    iterator erase( iterator first, iterator last )
    {
        iterator i = std::copy( last, finish, first );
        destroy( i, finish );
        finish = finish - ( last - first );
        return first;
    }
    // 从position开始，插入n个元素，元素初值为x
    void insert( iterator position, size_type n, const_reference x )
    {
        if( n == 0 )
            return;
        if( size_type( end_of_storage - finish ) >= n )
        {
            // 备用空间够
            T x_copy = x;
            // 计算插入点之后的现有元素个数
            const size_type elems_after = finish - position;
            iterator old_finish = finish;
            if( elems_after > n )
            {
                // 插入点后的元素大于新增元素个数
                std::uninitialized_copy( finish-n, finish, finish );
                finish += n;
                std::copy_backward( position, old_finish-n, old_finish );
                std::fill( position, position+n, x_copy );
            }
            else
            {
                uninitialized_fill_n( finish, n-elems_after, x_copy );
                finish += n - elems_after;
                std::uninitialized_copy( position, old_finish, finish );
                finish += elems_after;
                std::fill( position, old_finish, x_copy );
            }
        }
        else
        {
            // 备用空间小于新增元素个数，需决策新长度
            const size_type old_size = size();
            const size_type len = old_size + std::max( old_size, n );
            // 配置新的vector空间
            iterator new_start = data_allocator::allocate(len);
            iterator new_finish = new_start;
            // 插入点之前的元素复制到新空间
            new_finish = std::uninitialized_copy( start, position, new_start );
            // 将插入元素放入新空间
            new_finish = uninitialized_fill_n( new_finish, n, x );
            // 将插入点之后的元素复制到新空间
            new_finish = std::uninitialized_copy( position, finish, new_finish );

            // 清除并释放旧的vector
            destroy( start, finish );
            deallocate();

            // 维护新的迭代器
            start = new_start;
            finish = new_finish;
            end_of_storage = new_start + len;
        }

    }
    void resize( size_type new_size, const_reference x )
    {
        if( new_size < size() )
            erase( begin() + new_size, end() );
        else
            insert( end(), new_size - size(), x );
    }
    void resize( size_type new_size ) { resize( new_size, T() ); }
    void clear() { erase( begin(), end() ); }
};



#endif //SEQUENCE_CONTAINERS_SEQUENCE_CONTAINERS_H
