#include <filesystem>
#include <iostream>

#include <mdbx.h++>

#include "dbwrapper.h"
#include "util.h"
#include "mdbx.h"

// These are *impl* objects, defined here only to avoid dependencies in the header. (PIMPL)

struct MDBXBatch::MDBXWriteBatchImpl {
    mdbx::txn_managed txn;
    mdbx::map_handle map;
};

// Defined in the implementation file to avoid mdbx includes in the header, in
// accordance with the needs of libbitcoinkernel.

struct MDBXContext {
    mdbx::env::operate_parameters operate_params;
    mdbx::env_managed::create_parameters create_params;

    // MDBX read txn and map
    mdbx::txn_managed read_txn;
    // MDBX environment handle
    mdbx::env_managed env;
    mdbx::map_handle read_map;

    ~MDBXContext()
    {
        read_txn.abort();
        env.close();
    }
};

MDBXWrapper::MDBXWrapper(std::filesystem::path path)
    : CDBWrapperBase(DBParams{}),
    m_db_context{std::make_unique<MDBXContext>()}
{
    // initialize the mdbx environment.
    DBContext().env = mdbx::env_managed(path, DBContext().create_params, DBContext().operate_params);

    auto tempwrite = DBContext().env.start_write();
    auto tempmap = tempwrite.create_map(nullptr, mdbx::key_mode::usual, mdbx::value_mode::single);
    tempwrite.commit();

    DBContext().read_txn = DBContext().env.start_read();
    DBContext().read_map = DBContext().read_txn.open_map(nullptr, mdbx::key_mode::usual, mdbx::value_mode::single);
};

MDBXWrapper::~MDBXWrapper() = default;

void MDBXWrapper::Sync()
{
    DBContext().env.sync_to_disk();
}

std::optional<std::string> MDBXWrapper::ReadImpl(std::span<const std::byte> key) const
{
    // Used to represent key-not-found
    const mdbx::slice& not_found("key not found");
    mdbx::slice slKey(CharCast(key.data()), key.size()), slValue;
    slValue = DBContext().read_txn.get(DBContext().read_map, slKey, not_found);
    
    if(slValue == not_found) {
            return std::nullopt;
    }
    else {
        return std::string(slValue.as_string());
    }

    assert(-1);
}

bool MDBXWrapper::ExistsImpl(std::span<const std::byte> key) const
{
    mdbx::slice slKey(CharCast(key.data()), key.size()), slValue;
    slValue = DBContext().read_txn.get(DBContext().read_map, slKey, mdbx::slice::invalid());
    
    if(slValue == mdbx::slice::invalid()) {
            return false;
    }
    else {
        return true;
    }

    assert(-1);
}


bool MDBXWrapper::WriteBatch(CDBBatchBase& _batch, bool fSync)
{
    MDBXBatch& batch = static_cast<MDBXBatch&>(_batch);
    batch.m_impl_batch->txn.commit();

    if(fSync) {
        Sync();
    }

    return true;
}

size_t MDBXWrapper::EstimateSizeImpl(std::span<const std::byte> key1, std::span<const std::byte> key2) const
{
    // Only relevant for `gettxoutsetinfo` rpc.
    // Hint: (leaves + inner pages + overflow pages) * page size.
    return size_t{0};
}

size_t MDBXWrapper::DynamicMemoryUsage() const
{
    // Only relevant for some logging that happens in WriteBatch
    // TODO: how can I estimate this? I believe mmap makes this a challenge
    return size_t{0};
}

bool MDBXWrapper::IsEmpty()
{
    auto cursor{DBContext().read_txn.open_cursor(DBContext().read_map)};

    // the done parameter indicates whether or not the cursor move succeeded.
    return cursor.to_first(/*throw_notfound=*/false).done;
}

MDBXBatch::MDBXBatch (const CDBWrapperBase& _parent) : CDBBatchBase(_parent)
{
    const MDBXWrapper& parent = static_cast<const MDBXWrapper&>(m_parent);
    m_impl_batch = std::make_unique<MDBXWriteBatchImpl>();

    parent.DBContext().read_txn.reset_reading();
    // MDBXBatch is a wrapper for LMDB/MDBX's txn
    m_impl_batch->txn = parent.DBContext().env.start_write();
    m_impl_batch->map = m_impl_batch->txn.create_map(nullptr, mdbx::key_mode::usual, mdbx::value_mode::single);
};

MDBXBatch::~MDBXBatch()
{
    if(m_impl_batch->txn){
        m_impl_batch->txn.abort();
    }
    const MDBXWrapper& parent = static_cast<const MDBXWrapper&>(m_parent);

    parent.DBContext().read_txn.renew_reading();
}

void MDBXBatch::Clear()
{
    m_impl_batch->txn.abort();
    size_estimate = 0;
}

void MDBXBatch::WriteImpl(std::span<const std::byte> key, DataStream& ssValue)
{
    mdbx::slice slKey(CharCast(key.data()), key.size());
    //ssValue.Xor(dbwrapper_private::GetObfuscateKey(parent));
    mdbx::slice slValue(CharCast(ssValue.data()), ssValue.size());

    try {
        m_impl_batch->txn.put(m_impl_batch->map, slKey, slValue, mdbx::put_mode::upsert);
    }
    catch (mdbx::error err) {
        const std::string errmsg = "Fatal LevelDB error: " + err.message();
        std::cout << errmsg << std::endl;
        throw dbwrapper_error(errmsg);
    }

    // LevelDB serializes writes as:
    // - byte: header
    // - varint: key length (1 byte up to 127B, 2 bytes up to 16383B, ...)
    // - byte[]: key
    // - varint: value length
    // - byte[]: value
    // The formula below assumes the key and value are both less than 16k.
    // size_estimate += 3 + (slKey.size() > 127) + slKey.size() + (slValue.size() > 127) + slValue.size();
} 

void MDBXBatch::EraseImpl(std::span<const std::byte> key)
{
    mdbx::slice slKey(CharCast(key.data()), key.size());
    m_impl_batch->txn.erase(m_impl_batch->map, slKey);
    // LevelDB serializes erases as:
    // - byte: header
    // - varint: key length
    // - byte[]: key
    // The formula below assumes the key is less than 16kB.
    // size_estimate += 2 + (slKey.size() > 127) + slKey.size();
}

struct MDBXIterator::IteratorImpl {
    const std::unique_ptr<mdbx::cursor_managed> cursor;
    explicit IteratorImpl(mdbx::cursor_managed _cursor) : cursor{std::make_unique<mdbx::cursor_managed>(std::move(_cursor))} {}
};

MDBXIterator::MDBXIterator(const CDBWrapperBase& _parent, std::unique_ptr<IteratorImpl> _piter): CDBIteratorBase(_parent),
                                                                                            m_impl_iter(std::move(_piter)) {}

void MDBXIterator::SeekImpl(std::span<const std::byte> key)
{
    mdbx::slice slKey(CharCast(key.data()), key.size());
    m_impl_iter->cursor->seek(slKey);
}

CDBIteratorBase* MDBXWrapper::NewIterator()
{
    return new MDBXIterator{*this, std::make_unique<MDBXIterator::IteratorImpl>(DBContext().read_txn.open_cursor(DBContext().read_map))};
}

std::span<const std::byte> MDBXIterator::GetKeyImpl() const
{
    return std::as_bytes(m_impl_iter->cursor->current().key.bytes());
}

std::span<const std::byte> MDBXIterator::GetValueImpl() const
{
    // std::as_bytes is necessary since mdbx::slice::bytes() returns a span of `char8_` not std::byte
    return std::as_bytes(m_impl_iter->cursor->current().value.bytes());
}

MDBXIterator::~MDBXIterator() = default;

bool MDBXIterator::Valid() const {
    return false;
}

void MDBXIterator::SeekToFirst()
{
    m_impl_iter->cursor->to_first();
}

void MDBXIterator::Next()
{
    m_impl_iter->cursor->to_next();
}
