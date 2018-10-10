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

// node结构设计
template <class T>
struct __list_node
{
    typedef void* void_pointer;
    void_pointer prev;          // 型别为void*，足够灵活，其实可设为__list_node<T>*
    void_pointer next;
    T data;
};
// iterator结构设计
template <class T, class Ref, class Ptr>
struct __list_iterator
{
    typedef __list_iterator<T, T&, T*> iterator;
    typedef __list_iterator<T, Ref, Ptr> self;

    typedef std::bidirectional_iterator_tag iterator_category;
    typedef T value_type;
    typedef Ptr pointer;
    typedef Ref reference;
    typedef __list_node<T>* link_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    link_type node;     // 迭代器内部的普通指针

    // ctor
    __list_iterator(link_type x) : node(x){}
    __list_iterator(){}
    __list_iterator(const iterator& x) : node(x.node){}

    bool operator==(const self& x) const
    { return node == x.node; }
    bool operator!=(const self& x) const

    { return node != x.node; }
    reference operator*() const
    { return (*node).data; }
    pointer operator->() const
    { return &(operator*()); }
    // 对迭代器累加
    self& operator++()
    {
        node = (link_type)((*node).next);
        return *this;
    }
    self operator++(int)
    {
        self ret = *this;
        ++*this;
        return ret;
    }

    self& operator--()
    {
        node = (link_type)((*node).next);
        return *this;
    }
    self operator--(int)
    {
        self ret = *this;
        --*this;
        return ret;
    }
};
// list结构设计
// 只需要一个迭代器即可以表现
// 刻意设置一个空节点,满足STL前闭后开原则
template <class T, class Alloc = alloc>
class list
{
protected:
    typedef __list_node<T> list_node;               // 节点简称
    // 专属空间配置器，每次配置一个节点大小
    typedef simple_alloc<list_node, Alloc> list_node_allocator;

public:
    typedef list_node*      link_type;
    typedef __list_iterator<T,T&,T*> iterator;      // 重新命名为迭代器
    typedef size_t          size_type;
    typedef T       value_type;
    typedef T&      reference;
    typedef T*      pointer;

protected:
    link_type node;     // 只要一个指针,便可表示整个环状双向链表
    // 配置一个节点并返回
    link_type get_node() { return list_node_allocator::allocate(); }
    // 释放一个节点
    void put_node(link_type p ) { list_node_allocator::deallocate(p); }
    // 产生(配置并构造)一个节点,带有元素值
    link_type create_node( const T& x )
    {
        link_type p = get_node();
        construct( p, x );
        return p;
    }
    // 销毁(析构并释放)一个节点
    void destroy_node(link_type p)
    {
        destroy( p );
        put_node( p );
    }

public:
    // ctor
    list() { empty_initialize(); }
    // 插入一个节点,作为尾节点
    void push_back( const T& x )
    { insert( end(), x ); }
    // 插入一个节点,作为头节点
    void push_front( const T& x )
    { insert( begin(), x ); }

protected:
    void empty_initialize()
    {
        node = get_node();
        node->next = node;
        node->prev = node;
    }
    // 函数目的:在迭代器position所指位置插入一个节点,内容为x,返回指向新建节点的迭代器
    iterator insert( iterator position, const T& x )
    {
        link_type tmp = create_node( x );
        // 调整指针
        tmp->next = position.node;
        tmp->prev = position.node->prev;
        (link_type(position.node->prev))->next = tmp;
        return tmp;
    }

public:
    iterator begin() { return (link_type)((*node).next); }
    iterator end() { return node; }
    bool empty() const
    { return node->next == node; }
    size_type size() const
    {
        size_type result = 0;
        std::distance( begin(), end(), result );
        return result;
    }
    // 取头节点的内容
    reference front() { return *begin(); }
    // 取尾节点的内容
    reference back() { return *(--end()); }
    // 移除迭代器所指节点,返回下一个节点的迭代器
    iterator erase( iterator position )
    {
        // 记录移除节点的前后节点
        link_type next_node = position.node->next;
        link_type prev_node = position.node->prev;
        // 进行删除逻辑
        prev_node->next = next_node;
        next_node->prev = prev_node;
        // 销毁节点
        destroy_node( position );
        return iterator(next_node);
    }
    // 移除头节点
    void pop_front()
    { erase( begin() ); }
    // 移除尾节点
    void pop_back()
    { erase( end() ); }

    // 清除所有节点(整个链表)
    void clear()
    {
        link_type cur = (link_type) node->next;     // 开始节点
        while( cur != node )
        {
            link_type tmp = cur;
            cur = cur->next;
            destroy_node( tmp );
        }
        // 恢复node原始状态
        node->next = node;
        node->prev = node;
    }

    // 将数值为value的节点全部移除
    void remove( const T& value )
    {
        iterator first = begin();
        iterator last = end();
        while( first != last )
        {
            iterator next = first;
            ++next;
            if( *first == value ) destroy_node( first );
            first = next;
        }
    }

    // 移除数值相同的连续元素,"连续而相同的元素",才会被移除剩一个
    void unique()
    {
        iterator first = begin();
        iterator last = end();
        if( first == last ) return;     // 空链表什么都不做
        iterator next = first;
        while( ++next != last )
        {
            if( *first == *next )
                erase(next);
            else
                first = next;
            next = first;
        }
    }

protected:
    // 将[first,last)内的所有元素移动到position之前
    void transfer( iterator position, iterator first, iterator last )
    {
        // 思路就是每变动一个指针,就会出现两个指针指向同一个节点情况,处理那个未变动的指针指向正确的位置
        // 分为向前处理与向后处理两个逻辑
        if( position != last )
        {
            (*(link_type((*last.node).prev))).next = position.node;
            (*link_type((*position.node).prev)).next = first.node;
            (*link_type((*first.node).prev)).next = last.node;
            link_type tmp = link_type((*position.node).prev);
            (*position.node).prev = (*last.node).prev;
            (*last.node).prev = (*first.node).prev;
            (*first.node).prev = tmp;
        }
    }

public:
    // 接合操作
    // 将x结合于position所指位置之前,x必须不同于*this
    void splice( iterator position, list& x )
    {
        if( !x.empty() )
            transfer( position, x.begin(), x.end() );
    }

    // 将i所指元素接合于position所指位置之前,position和i可能指向同一个list
    void splice( iterator position, list&, iterator i )
    {
        iterator j = i;
        ++j;
        if( position == i || position == j ) return;    // 要么已经在前面了,要么是插入位置是它自身
        transfer( position, i, j );
    }

    // 将[first,last)内的所有元素接合于position所指之前
    // position和区间可能在同一list中
    // 但position不能在区间中
    void splice( iterator position, list&, iterator first, iterator last )
    {
        if( first != last )
            transfer( position, first, last );
    }

    // 将x合并到*this上,两个list必须都已经过递增排序
    void merge( list& x )
    {
        iterator first1 = begin();
        iterator last1 = end();
        iterator first2 = x.begin();
        iterator last2 = x.end();

        while( first1 != last1 && first2 != last2 )
        {
            if( *first1 < *first2 )
            {
                iterator next = first2;
                transfer( first1, first2, ++next );
                first2 = next;
            }
            else
                ++first1;
            if( first2 != last2 )       // TODO
                transfer( last1, first2, last2 );
        }
    }
};



#endif //SEQUENCE_CONTAINERS_SEQUENCE_CONTAINERS_H
