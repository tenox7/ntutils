#ifndef FTE_STL_H
#define FTE_STL_H

// some basic types for our C++ usage
// we are replacing overbloated STL

//#define CONFIG_FTE_USE_STL

#ifndef CONFIG_FTE_USE_STL
#include <inttypes.h>
#include <sys/types.h>
#include <assert.h>

#define FTE_BEGIN_NAMESPACE namespace fte {
#define FTE_END_NAMESPACE }

#if defined(__GNUC__) && \
    ((__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ > 4)))
#   define _fte_printf_attr(a,b) __attribute((__format__(__printf__,a,b)))
#else
#   define _fte_printf_attr(a,b)
#endif

//#include <stdio.h>

FTE_BEGIN_NAMESPACE;

/**
 * Simple class for storing SmartPointers
 */
template <class T, bool ARRAY = false> class aptr
{
public:
    explicit aptr(T* p) : pointer(p) {}
    ~aptr()
    {
	if (ARRAY)
	    delete [] pointer ;
	else
	    delete pointer;
    }
    T& operator*() const { return *pointer; }
    T* operator->() const { return pointer; }
private:
    T* pointer;
};

/**
 * Class to be used for swapping two variable of the same type
 */
template <class T>
inline void swap(T& a, T& b)
{
    const T tmp = a;
    a = b;
    b = tmp;
}

/**
 * Class to be used instead of line sequence:
 *   delete p;
 *   p = 0;
 */
//template <class T> inline void destroy(T*& p) { delete p; p = 0; }


/**
 * Simple class for storing char*
 *
 *
 * The main reason for existance of this class is faster compilation.
 * We do not need overcomplicated  std::string class for our purpose.
 * The behaviour of implemented methods should mostly match them 1:1
 */
class string
{
public:
    typedef size_t size_type;
    static const size_type npos = ~0U;

    string() : str(empty_string) {}
    string(const char* s);
    string(const char* s, size_type len);
    string(const string& s);
    string(const string& s, size_type len);
    ~string();
    char operator[](size_type i) const { return str[i]; }
    char& operator[](size_type i) { return str[i]; }
    bool operator==(const char* s) const;
    bool operator==(const string& s) const { return operator==(s.str); }
    bool operator!=(const char* s) const { return !operator==(s); }
    bool operator!=(const string& s) const { return !operator==(s); }
    bool operator<(const string& s) const;
    string operator+(char c) const;
    string operator+(const char* s) const;
    string operator+(const string& s) const;
    string& operator=(char c);
    string& operator=(const char* s);
    string& operator=(const string& s) { return (this == &s) ? *this : operator=(s.str); }
    string& operator+=(char c) { return append(c); }
    string& operator+=(const char* s) { return append(s); }
    string& operator+=(const string& s) { return append(s.str); }
    string& append(char c);
    string& append(const char* s);
    string& append(const string& s) { return append(s.str); }
    char* begin() { return str; }
    const char* begin() const { return str; }
    void clear() { string tmp; swap(tmp); }
    const char* c_str() const { return str; }
    char* end() { return str + size(); }
    const char* end() const { return str + size(); }
    bool empty() const { return str[0] == 0; }
    size_type find(const string& s, size_type startpos = 0) const;
    size_type find(char c) const;
    size_type rfind(char c) const;
    void insert(size_type pos, const string& s);
    string& erase(size_type pos = 0, size_type n = npos);
    size_type size() const { return slen(str); }
    string substr(size_type from = 0, size_type to = npos) const { return string(str + from, to); };
    void swap(string& s) { fte::swap(str, s.str); }

    /* string extensions */
    string(char s);
    int sprintf(const char* fmt, ...) _fte_printf_attr(2, 3); // allocates size
    // it will use just 1024 bytes for non _GNU_SOURCE compilation!!
    string& tolower();
    string& toupper();

private:
    char* str;
    static char* empty_string;
    static inline size_type slen(const char* s) { size_type i = 0; while (s[i]) ++i; return i; }
    string(const char* s1, size_type sz1, const char* s2, size_type sz2);
};

template<> inline void swap(fte::string& a, fte::string& b) { a.swap(b); }

/*
 * without this operator attempt to compare const char* with string will give quite unexpected
 * results because of implicit usage of operator const char*() with the right operand
 */
inline bool operator==(const char* s1, const string& s2) { return s2 == s1; }


/**
 * Simple vector class
 *
 * Implemented methods behaves like std::vector
 */
template <class Type> class vector
{
public:
    typedef const Type* const_iterator;
    typedef Type* iterator;
    typedef const Type& const_reference;
    typedef Type& reference;
    typedef size_t size_type;

    static const size_type invalid = ~0U;

    vector<Type>() :
	m_begin(0), m_capacity(m_begin), m_end(m_begin)
    {
    }

    vector<Type>(size_type prealloc) :
	m_begin(prealloc ? new Type[prealloc] : 0),
	m_capacity(m_begin + prealloc), m_end(m_begin)
    {
    }

    // we will not count references - we have to program with this in mind!
    vector<Type>(const vector<Type>& t) :
	m_begin(0), m_capacity(m_begin), m_end(m_begin)
    {
	copy(t.m_begin, t.m_end, t.size());
    }
    vector<Type>& operator=(const vector<Type>& t)
    {
	if (this != &t) {
	    vector<Type> tmp(t);
	    swap(tmp);
	}
	return *this;
    }
    ~vector();
    const_reference operator[](size_type i) const { return m_begin[i]; }
    reference operator[](size_type i) { return m_begin[i]; }
    const_iterator begin() const { return m_begin; }
    iterator begin() { return m_begin; }
    const_reference front() const { return m_begin[0]; }
    reference front() { return m_begin[0]; }
    const_iterator end() const { return m_end; }
    iterator end() { return m_end; }
    const_reference back() const { return m_end[-1]; }
    reference back() { return m_end[-1]; }
    size_type capacity() const { return m_end - m_begin; }
    void clear() { vector<Type> tmp; swap(tmp); }
    bool empty() const { return (m_begin == m_end); }
    iterator erase(iterator pos) { return erase(pos, pos + 1); }
    iterator erase(iterator first, iterator last);
    iterator insert(iterator pos, const Type& t)
    {
	const size_type n = pos - m_begin;
	insert(pos, &t, &t + 1);
	return m_begin + n;
    }
    void insert(iterator pos, const_iterator from, const_iterator to);
    void pop_back()
    {
	//printf("vector pop_back %d\n", m_size);
	assert(m_begin != m_end);
        --m_end;
	if (size() < capacity() / 4)
	    internal_copy(m_begin, m_end, size() * 2);
    }
    void pop_front()
    {
	assert(m_begin != m_end);
        --m_end;
	if (size() < capacity() / 4)
	    internal_copy(m_begin + 1, m_end, size() * 2);
        else
	    for (iterator it = m_begin; it != m_end; ++it)
		internal_swap(it[0], it[1]);
    }
    void push_back(const_reference m)
    {
	if (m_end == m_capacity)
	    internal_copy(m_begin, m_end, capacity() * 2);
	*m_end++ = m;
    }
    void reserve(size_type sz)
    {
	if (sz > capacity())
	    internal_copy(m_begin, m_end, sz);
    }
    void resize(size_type sz)
    {
	internal_copy(m_begin, (sz < size()) ? m_begin + sz : m_end, sz);
        m_end = m_begin + sz; // even empty members
    }
    size_type size() const { return m_end - m_begin; }
    void swap(vector<Type>& v)
    {
	fte::swap(m_begin, v.m_begin);
	fte::swap(m_capacity, v.m_capacity);
	fte::swap(m_end, v.m_end);
    }

    /* vector extensions */
    size_type find(const_reference t) const
    {
	for (iterator it = m_begin; it != m_end; ++it)
	    if (t == *it)
		return (it - m_begin);
	return invalid;
    }
    void remove(const_reference t);

protected:
    static const size_type min_capacity = 4;
    Type* m_begin;
    Type* m_capacity;
    Type* m_end;
    void copy(const_iterator first, const_iterator last, size_type alloc);
    void internal_copy(iterator first, iterator last, size_type alloc);
    void internal_swap(Type& a, Type& b) { a = b; }
};

template <class Type>
vector<Type>::~vector()
{
    delete[] m_begin;
}

template <class Type>
void vector<Type>::copy(const_iterator first, const_iterator last, size_type alloc)
{
    assert(size() <= alloc);
    vector<Type> tmp((alloc < min_capacity) ? min_capacity : alloc);
    for (; first != last; ++tmp.m_end, ++first)
	*tmp.m_end = *first;
    swap(tmp);
}

template <class Type>
void vector<Type>::internal_copy(iterator first, iterator last, size_type alloc)
{
    assert(size() <= alloc);
    vector<Type> tmp((alloc < min_capacity) ? min_capacity : alloc);
    for (; first != last; ++tmp.m_end, ++first)
	internal_swap(*tmp.m_end, *first);
    swap(tmp);
}

template <class Type>
typename vector<Type>::iterator vector<Type>::erase(iterator first, iterator last)
{
    assert(last <= m_end);
    m_end -= (last - first);
    for (iterator it = first; it != m_end; ++last, ++it)
	internal_swap(*it, *last);
    return first;
}

template <class Type>
void vector<Type>::insert(iterator pos, const_iterator from, const_iterator to)
{
    size_type isz = to - from;
    if (!isz)
	return;

    Type* tmp;

    if (m_end + isz < m_capacity)
	tmp = m_begin;
    else
    {
	size_type nc = ((size() + isz > min_capacity)
			? size() + isz : min_capacity) * 2;
	tmp = new Type[nc];
	m_capacity = tmp + nc;
    }

    iterator l = m_end;
    m_end = tmp + size() + isz;
    if (l)
	for (iterator it = m_end; --l >= pos;)
	    internal_swap(*--it, *l);

    size_type n = pos - m_begin;
    for (iterator it = tmp + n; from != to; ++it, ++from)
	*it = *from;

    if (tmp != m_begin)
    {
	for (size_type i = 0; i < n; ++i)
	    internal_swap(tmp[i], m_begin[i]);
	delete[] m_begin;
	m_begin = tmp;
    }
}

template <class Type>
void vector<Type>::remove(const_reference t)
{
    iterator from = m_begin;
    for (iterator it = from; it != m_end; ++it)
	if (t != *it)
	{
	    if (from != it)
		internal_swap(*from, *it);
	    ++from;
	}
    m_end = from;
}

template <class Type>
inline void swap(fte::vector<Type>& a, fte::vector<Type>& b) { a.swap(b); }

/*
 * partial specialization for some common types
 * for more effective transfers in copy constructions
 * instead of having two copies - we allow to swap between field values
 * thus we can maintain vector<string> without reallocation of string with
 * every size change of vector<>
 */
template <>
inline void vector<fte::string>::internal_swap(fte::string& a, fte::string& b) { a.swap(b); }


#define StlString   fte::string
#define StlVector   fte::vector

FTE_END_NAMESPACE;

#else

#include <string>
#include <vector>

#define StlString   std::string
#define StlVector   std::vector

#endif // CONFIG_FTE_USE_STL

#define vector_const_iterate(type, var, i) for (StlVector<type>::const_iterator i = var.begin(); i != var.end(); ++i)
#define vector_iterate(type, var, i) for (StlVector<type>::iterator i = var.begin(); i != var.end(); ++i)

#endif // FTE_STL_H
