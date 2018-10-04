#include <iostream>
#include <string>




template <typename T>
class auto_ptr
{
private:
    T* pointee;
public:
    explicit auto_ptr( T* p = 0 ): pointee( p ){}
    ~auto_ptr(){ delete( pointee ); }

    T& operator*() const { return *pointee; }
    T& operator*( int num ) const
    {
        *pointee = *pointee * num;
        return *pointee;
    }
    T* operator->() const { return pointee; }
};

// 这是一个自定义类T的迭代器
template <class T>
struct MyIter
{
    typedef T value_type;       // 内嵌型别声明——即在迭代器内声明了型别
    T* ptr;
    MyIter( T* p = 0 ) : ptr( p ){}
    T& operator*() const { return *ptr; }
    // ...
};

// 取得迭代器的型别
template <class I>
typename I::value_type      // 这一整行是func的返回值类型
func( I ite )
{ return *ite; }

// 以上方法 不适用于原生指针  因为原生指针不需要额外定义

// 5个标记用的型别
// 只作为标记用，不需要任何成员
// 继承关系的使用是为了去掉单纯的传递调用
struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

// 解决方法，偏特化，额外加一层间接关系

// 特性萃取机，迭代器必须遵守约定，自行以内嵌型别定义

template <typename I>   // 泛化版本，传入一个迭代器
struct iterator_traits
{
    typedef typename I::value_type          value_type;         // 迭代器型别
    typedef typename I::iterator_category   iterator_category;  // 迭代器类型
    typedef typename I::difference_type     difference_type;    // 两个迭代器间的距离
    typedef typename I::pointer             pointer;            // 指向迭代器所指之物（指针）
    typedef typename I::reference           reference;          // 迭代器所指之物(引用)
};

template <class I>
inline typename iterator_traits<I>::iterator_category
iterator_category( const I& )
{
    typedef typename iterator_traits<I>::iterator_category category;
    return category();
}

template <typename T>   // 特化版本1，传入原生指针
struct iterator_traits<T*>
{
    typedef T           value_type;     // 直接使用T即可
    typedef ptrdiff_t   difference_type;
    typedef T&          reference;
    typedef T*          pointer;
    typedef random_access_iterator_tag  iterator_category;
};

template <typename T>   // 特化版本2，传入const型原生指针
struct iterator_traits<const T*>
{
    typedef T           value_type;     // 依旧直接使用T
    typedef ptrdiff_t   difference_type;
    typedef T&          reference;
    typedef T*          pointer;
    typedef random_access_iterator_tag  iterator_category;
};

// 迭代器分类和函数重载
template <class InputIterator, class Distance>
inline void __advance( InputIterator& i, Distance n, input_iterator_tag )
{
    // 单向，逐一前进
    while( n-- ) ++i;
}
// 单纯的传递调用函数，可以不要，因为继承关系，不匹配时，自动向上层（基类）转换
/*template <class ForwardIterator, class Distance>
inline void __advance( ForwardIterator& i, Distance n, forward_iterator_tag )
{
    __advance( i, n, input_iterator_tag() );
}*/
template <class BidiectionalIterator, class Distance>
inline void __advance( BidiectionalIterator& i, Distance n, bidirectional_iterator_tag )
{
    if( n >= 0 )
        while( n-- ) ++i;
    else
        while( n++ ) --i;
}
template <class RandomAccessIterator, class Distance>
inline void __advance( RandomAccessIterator& i, Distance n, random_access_iterator_tag )
{
    // 双向
     i += n;
}
// 上层接口
// STL算法命名规则：以算法能接受之最低阶迭代器类型命名
template <class InputIterator, class Distance>
inline void __advance( InputIterator& i, Distance n )
{
    __advance( i, n, iterator_traits<InputIterator>::iterator_category() );
}


// 设计迭代器时，直接继承此结构，可保证符合STL架构
template<class Category, class T, class Distance = ptrdiff_t, class Pointer=T*, class Reference = T&>
struct iterator
{
        typedef Category		iterator_category;
        typedef T			value_type;
        typedef Distance		difference_type;
        typedef Pointer		pointer;
        typedef Reference	reference;
};



int main()
{
    auto_ptr<int> pi( new int(5) );
    int num = 5;
    std::cout << *pi << std::endl;
    std::cout << pi * 5 << std::endl;
    std::cout << *pi << std::endl;
    return 0;
}

