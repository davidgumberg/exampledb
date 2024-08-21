#include <cassert>
#include <filesystem>
#include <mdbx.h>

#include "dbwrapper.h"

// We avoid including mdbx.h++ in this file, to follow the constraints of
// bitcoin core, we don't want library users to have to include symbols

// MDBXContext is defined in mdbx.cpp to avoid dependency on libmdbx here
struct MDBXContext;

class MDBXBatch;

class MDBXWrapper : public CDBWrapperBase
{
private:
    std::unique_ptr<MDBXContext> m_db_context;

    auto& DBContext() const [[clang::lifetimebound]] {
        assert(m_db_context);
        return *m_db_context;

    }

    std::optional<std::string> ReadImpl(std::span<const std::byte> key) const override;
    bool ExistsImpl(std::span<const std::byte> key) const override;
    size_t EstimateSizeImpl(std::span<const std::byte> key1, std::span<const std::byte> key2) const override;

    std::unique_ptr<CDBBatchBase> CreateBatch() const override;

public:
    MDBXWrapper(std::filesystem::path path);
    ~MDBXWrapper() override;

    bool WriteBatch(CDBBatchBase& batch, bool fSync) override;

    // Get an estimate of MDBX memory usage (in bytes).
    size_t DynamicMemoryUsage() const override;

    CDBIteratorBase* NewIterator() override;

    /**
     * Return true if the database managed by this class contains no entries.
     */
    bool IsEmpty() override;
};

/** Batch of changes queued to be written to an MDBXWrapper */
class MDBXBatch : public CDBBatchBase
{
    friend class MDBXWrapper;

private:
    const MDBXWrapper &parent;

    struct WriteBatchImpl;
    const std::unique_ptr<WriteBatchImpl> m_impl_batch;

    void WriteImpl(std::span<const std::byte> key, DataStream& ssValue) override;
    void EraseImpl(std::span<const std::byte> key) override;

public:
    /**
     * @param[in] _parent   CDBWrapper that this batch is to be submitted to
     */
    explicit MDBXBatch(const CDBWrapperBase& _parent);
    ~MDBXBatch();
    void Clear() override;
};

/** Batch of changes queued to be written to a CDBWrapper */
class MDBXIterator : public CDBIteratorBase
{
public:
    struct IteratorImpl;
    private:
    const std::unique_ptr<IteratorImpl> m_impl_iter;

    void SeekImpl(std::span<const std::byte> key) override;
    std::span<const std::byte> GetKeyImpl() const override;
    std::span<const std::byte> GetValueImpl() const override;

public:

    /**
     * @param[in] _parent          Parent CDBWrapper instance.
     * @param[in] _piter           MDBX iterator.
     */
    MDBXIterator(const CDBWrapperBase& _parent, std::unique_ptr<IteratorImpl> _piter);
    ~MDBXIterator() override;

    bool Valid() const override;
    void SeekToFirst() override;
    void Next() override;
};
