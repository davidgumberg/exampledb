#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <cstring>
#include <ios>
#include <span>
#include <vector>

// Used for slices
static auto CharCast(const std::byte* data) { return reinterpret_cast<const char*>(data); }

template<typename Stream> inline void ser_writedata8(Stream &s, uint8_t obj)
{
    s.write(std::as_bytes(std::span{&obj, 1}));
}
template <typename Stream> void Serialize(Stream& s, std::byte a) { ser_writedata8(s, uint8_t(a)); }

template<typename Stream> inline uint8_t ser_readdata8(Stream &s)
{
    uint8_t obj;
    s.read(std::as_writable_bytes(std::span{&obj, 1}));
    return obj;
}
template <typename Stream> void Unserialize(Stream& s, std::byte& a) { a = std::byte{ser_readdata8(s)}; }



// A simplified reimplementation of Bitcoin Core's DataStream class that
// provides a stream-like interface to a vector.

class DataStream
{
protected:
    // A DataStream is a vector of bytes.
    using vector_type = std::vector<std::byte>;
    vector_type vch;
    // What does the m_read_pos do?
    vector_type::size_type m_read_pos{0};

public:
    // type alias in all of the types std::vector makes available
    typedef vector_type::allocator_type   allocator_type;
    typedef vector_type::size_type        size_type;
    typedef vector_type::difference_type  difference_type;
    typedef vector_type::reference        reference;
    typedef vector_type::const_reference  const_reference;
    typedef vector_type::value_type       value_type;
    typedef vector_type::iterator         iterator;
    typedef vector_type::const_iterator   const_iterator;
    typedef vector_type::reverse_iterator reverse_iterator;

    // Default constructor
    explicit DataStream() = default;
    // Copy construct from a std::span of the vector's type (bytes).
    explicit DataStream(std::span<const value_type> sp) : vch(sp.data(), sp.data() + sp.size()) {}

    //
    // Subset of vector operations
    // Here you can see the essential difference between DataStream and an
    // std::vector, the presence of the m_read_pos in all the below methods.
    //

    // The DataStream's beginning is the vch's beginning + m_read_pos index;
    const_iterator begin() const                        { return vch.begin() + m_read_pos; }
    iterator begin()                                    { return vch.begin() + m_read_pos; }

    // The Datastream's end is the vch's end.
    const_iterator end() const                          { return vch.end(); }
    iterator end()                                      { return vch.end(); }

    size_type size() const                              { return vch.size() - m_read_pos; }
    bool empty() const                                  { return vch.size() == m_read_pos; }

    // resize to the requested size plus the read position.
    void resize(size_type n, value_type c = value_type{}) { vch.resize(n + m_read_pos, c); }
    void reserve(size_type n)                           { vch.reserve(n + m_read_pos); }
    const_reference operator[](size_type pos) const     { return vch[pos + m_read_pos]; }
    reference operator[](size_type pos)                 { return vch[pos + m_read_pos]; }

    // clear the vector and reset the read position.
    void clear()                                        { vch.clear(); m_read_pos = 0; }

    value_type* data()                                  { return vch.data() + m_read_pos; }
    const value_type* data() const                      { return vch.data() + m_read_pos; }

    // delete pre-read position index data
    inline void Compact()
    {
        vch.erase(vch.begin(), vch.begin() + m_read_pos);
        m_read_pos = 0;
    }

    //
    // Subset of stream operations
    //
    bool eof() const        { return size() == 0; }

    // " obtains the number of characters immediately available in the get area "
    int in_avail() const    { return size(); }

    void read(std::span<value_type> dst)
    {
        // do nothing if we're reading into any empty span.
        if (dst.size() == 0) return;

        // next_read_pos represents how far we'll have to read into our vch.
        auto next_read_pos{m_read_pos + dst.size()}; // This is a `CheckedAdd` in core.
        // make sure we don't overread vch
        if (next_read_pos > vch.size()) {
            throw std::ios_base::failure("DataStream::read(): end of data");
        }

        // Copy from vch into the destination buffer.
        memcpy(dst.data(), &vch[m_read_pos], dst.size());

        // We are iterating m_read_pos in this method,
        // handle the case where we have read up to the end of the vcch.
        if (next_read_pos == vch.size()) {
            m_read_pos = 0;
            vch.clear();
            return;
        }
        m_read_pos = next_read_pos;
    }

    void write(std::span<const value_type> src)
    {
        // Write to the end of the buffer
        vch.insert(vch.end(), src.begin(), src.end());
    }
    
    template<typename T>
    DataStream& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return (*this);
    }

    template<typename T>
    DataStream& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return (*this);
    }
};
#endif // UTIL_H
