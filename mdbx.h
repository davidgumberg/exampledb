#include <cassert>
#include <filesystem>
#include <mdbx.h>
#include <optional>

// We avoid including mdbx.h++ in this file, to follow the constraints of
// bitcoin core, we don't want library users to have to include symbols

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

class DBWrapperBase
{
protected:
    DBWrapperBase(const DBParams& params)
        : m_name(PathToString(params.path.stem())),
          m_path(params.path),
          m_is_memory(params.memory_only)
    {}

    //! the name of this database
    std::string m_name;

    //! path to filesystem storage
    const std::filesystem::path m_path;

    //! whether or not the database resides in memory
    bool m_is_memory;

public:
    virtual ~DBWrapperBase() = default;
};

// MDBXContext is defined in mdbx.cpp to avoid dependency on libmdbx here
struct MDBXContext;

class MDBXWrapper : DBWrapperBase
{
private:
    std::unique_ptr<MDBXContext> m_db_context;

    auto& DBContext() const [[clang::lifetimebound]] {
        assert(m_db_context);
        return *m_db_context;
    }
public:
    MDBXWrapper(std::filesystem::path path);
    ~MDBXWrapper() override;
};

/** Batch of changes queued to be written to an MDBXWrapper */
class MDBXBatch
{
    friend class MDBXWrapper;

private:
    const MDBXWrapper &parent;

    struct WriteBatchImpl;
    const std::unique_ptr<WriteBatchImpl> m_impl_batch;

    size_t size_estimate{0};

public:
    /**
     * @param[in] _parent   CDBWrapper that this batch is to be submitted to
     */
    explicit MDBXBatch(const DBWrapperBase& _parent);
    ~MDBXBatch();
    void Clear();

    size_t SizeEstimate() const { return size_estimate; }
};
