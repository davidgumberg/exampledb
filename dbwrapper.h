#ifndef DBWRAPPER_H
#define DBWRAPPER_H

#include <filesystem>
#include <optional>
#include <span>

#include "util.h"

// Most of this should be verbatim from https://github.com/davidgumberg/bitcoin/tree/adb/src/dbwrapper.h

static const size_t DBWRAPPER_PREALLOC_KEY_SIZE = 64;
static const size_t DBWRAPPER_PREALLOC_VALUE_SIZE = 1024;

// Attempt to closely match the base classes we'll be deriving from in bitcoin core.

//! User-controlled performance and debug options.
struct DBOptions {
    //! Compact database on startup.
    bool force_compact = false;
};

//! Application-specific storage settings.
struct DBParams {
    //! Location in the filesystem where leveldb data will be stored.
    std::filesystem::path path;
    //! Configures various leveldb cache settings.
    size_t cache_bytes;
    //! If true, use leveldb's memory environment.
    bool memory_only = false;
    //! If true, remove all existing data.
    bool wipe_data = false;
    //! If true, store data obfuscated via simple XOR. If false, XOR with a
    //! zero'd byte array.
    bool obfuscate = false;
    //! Passed-through options.
    DBOptions options{};
};

static inline std::string PathToString(const std::filesystem::path path)
{
    return path.std::filesystem::path::string();
}

class CDBWrapperBase;

/** Batch of changes queued to be written to a CDBWrapper */
class CDBBatchBase
{
protected:
    const CDBWrapperBase &parent;

    DataStream ssKey{};
    DataStream ssValue{};

    size_t size_estimate{0};

    virtual void WriteImpl(std::span<const std::byte> key, DataStream& ssValue) = 0;
    virtual void EraseImpl(std::span<const std::byte> key) = 0;

public:
    /**
     * @param[in] _parent   CDBWrapper that this batch is to be submitted to
     */
    explicit CDBBatchBase(const CDBWrapperBase& _parent) : parent{_parent} {}
    virtual ~CDBBatchBase() = default;
    virtual void Clear() = 0;

    size_t SizeEstimate() const { return size_estimate; }

    template <typename K, typename V>
    void Write(const K& key, const V& value)
    {
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssValue.reserve(DBWRAPPER_PREALLOC_VALUE_SIZE);
        ssKey << key;
        ssValue << value;
        WriteImpl(ssKey, ssValue);
        ssKey.clear();
        ssValue.clear();
    }

    template <typename K>
    void Erase(const K& key)
    {
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        EraseImpl(ssKey);
        ssKey.clear();
    }
};

class CDBIteratorBase;

class CDBWrapperBase
{
    // friend const std::vector<unsigned char>& dbwrapper_private::GetObfuscateKey(const CDBWrapperBase &w);

protected:
    CDBWrapperBase(const DBParams& params)
        : m_name(PathToString(params.path.stem())),
          m_path(params.path),
          m_is_memory(params.memory_only)
    {
//        obfuscate_key = CreateObfuscateKey();
    }

    //! the name of this database
    std::string m_name;

/*
    //! a key used for optional XOR-obfuscation of the database
    std::vector<unsigned char> obfuscate_key;

    //! the key under which the obfuscation key is stored
    static const std::string OBFUSCATE_KEY_KEY;

    //! the length of the obfuscate key in number of bytes
    static const unsigned int OBFUSCATE_KEY_NUM_BYTES;

    std::vector<unsigned char> CreateObfuscateKey() const;
*/

    //! path to filesystem storage
    const std::filesystem::path m_path;

    //! whether or not the database resides in memory
    bool m_is_memory;

    virtual std::optional<std::string> ReadImpl(std::span<const std::byte> key) const = 0;
    virtual bool ExistsImpl(std::span<const std::byte> key) const = 0;
    virtual size_t EstimateSizeImpl(std::span<const std::byte> key1, std::span<const std::byte> key2) const = 0;

    virtual std::unique_ptr<CDBBatchBase> CreateBatch() const = 0;

public:
    CDBWrapperBase(const CDBWrapperBase&) = delete;
    CDBWrapperBase& operator=(const CDBWrapperBase&) = delete;

    virtual ~CDBWrapperBase() = default;

    template <typename K, typename V>
    bool Read(const K& key, V& value) const
    {
        DataStream ssKey{};
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        std::optional<std::string> strValue{ReadImpl(ssKey)};
        if (!strValue) {
            return false;
        }
        try {
            DataStream ssValue{std::as_bytes(std::span{(*strValue)})};
            // ssValue.Xor(obfuscate_key);
            ssValue >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    template <typename K, typename V>
    bool Write(const K& key, const V& value, bool fSync = false)
    {
        auto batch = CreateBatch();
        batch->Write(key, value);
        return WriteBatch(*batch, fSync);
    }

    //! @returns filesystem path to the on-disk data.
    std::optional<std::filesystem::path> StoragePath() {
        if (m_is_memory) {
            return {};
        }
        return m_path;
    }

    template <typename K>
    bool Exists(const K& key) const
    {
        DataStream ssKey{};
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        return ExistsImpl(ssKey);
    }

    template <typename K>
    bool Erase(const K& key, bool fSync = false)
    {
        auto batch = CreateBatch();
        batch->Erase(key);
        return WriteBatch(*batch, fSync);
    }

    virtual bool WriteBatch(CDBBatchBase& batch, bool fSync) = 0;

    // Get an estimate of LevelDB memory usage (in bytes).
    virtual size_t DynamicMemoryUsage() const = 0;

    virtual CDBIteratorBase* NewIterator() = 0;

    /**
     * Return true if the database managed by this class contains no entries.
     */
    virtual bool IsEmpty() = 0;

    template<typename K>
    size_t EstimateSize(const K& key_begin, const K& key_end) const
    {
        DataStream ssKey1{}, ssKey2{};
        ssKey1.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey2.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey1 << key_begin;
        ssKey2 << key_end;
        return EstimateSizeImpl(ssKey1, ssKey2);
    }
};

class CDBIteratorBase
{
protected:
    const CDBWrapperBase &parent;

    virtual void SeekImpl(std::span<const std::byte> key) = 0;
    virtual std::span<const std::byte> GetKeyImpl() const = 0;
    virtual std::span<const std::byte> GetValueImpl() const = 0;
public:
    explicit CDBIteratorBase(const CDBWrapperBase& _parent)
        : parent(_parent) {}
    virtual ~CDBIteratorBase() = default;


    template<typename K> void Seek(const K& key) {
        DataStream ssKey{};
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        SeekImpl(ssKey);
    }

    template<typename K> bool GetKey(K& key) {
        try {
            DataStream ssKey{GetKeyImpl()};
            ssKey >> key;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    template<typename V> bool GetValue(V& value) {
        try {
            DataStream ssValue{GetValueImpl()};
            // ssValue.Xor(dbwrapper_private::GetObfuscateKey(parent));
            ssValue >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    virtual bool Valid() const = 0;
    virtual void SeekToFirst() = 0;
    virtual void Next() = 0;
};

#endif // DBWRAPPER_H
