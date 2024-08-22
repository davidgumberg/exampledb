#include <filesystem>
#include <iostream>

#include <mdbx.h++>

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

    // MDBX environment handle
    mdbx::env_managed env;
    // MDBX read txn and map
    mdbx::txn_managed read_txn;
    mdbx::map_handle read_map;

    ~MDBXContext()
    {
        env.close();
    }
};

MDBXWrapper::MDBXWrapper(std::filesystem::path path)
    : CDBWrapperBase(DBParams{}),
    m_db_context{std::make_unique<MDBXContext>()}
{
    // initialize the mdbx environment.
    DBContext().env = mdbx::env_managed(path, DBContext().create_params, DBContext().operate_params);
    DBContext().read_txn = DBContext().env.start_read();
    DBContext().read_map = DBContext().read_txn.create_map(nullptr, mdbx::key_mode::usual, mdbx::value_mode::single);
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
        return slValue.as_string();
    }

    if (!status.ok()) {
        if (status.IsNotFound())
            return std::nullopt;
        LogPrintf("LevelDB read failure: %s\n", status.ToString());
        HandleError(StatusImpl{status});
    }
    return strValue;
}

bool CDBWrapper::ExistsImpl(Span<const std::byte> key) const
{
    leveldb::Slice slKey(CharCast(key.data()), key.size());

    std::string strValue;
    leveldb::Status status = DBContext().pdb->Get(DBContext().readoptions, slKey, &strValue);
    if (!status.ok()) {
        if (status.IsNotFound())
            return false;
        LogPrintf("LevelDB read failure: %s\n", status.ToString());
        HandleError(StatusImpl{status});
    }
    return true;
}


bool MDBXWrapper::WriteBatch(CDBBatchBase& _batch, bool fSync)
{
    // Cast the base batch to derived MDBXBatch
    MDBXBatch& batch = static_cast<MDBXBatch&>(_batch);
    batch.m_impl_batch->txn.commit();

    if(fSync) {
        Sync();
    }
    return true;
}

MDBXBatch::MDBXBatch (const CDBWrapperBase& _parent) : CDBBatchBase(_parent)
{
    const MDBXWrapper& parent = static_cast<const MDBXWrapper&>(m_parent);

    // MDBXBatch is a wrapper for LMDB/MDBX's txn
    m_impl_batch->txn = parent.DBContext().env.start_write();
    m_impl_batch->map = m_impl_batch->txn.create_map(nullptr, mdbx::key_mode::usual, mdbx::value_mode::single);
};

MDBXBatch::~MDBXBatch() = default;

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
