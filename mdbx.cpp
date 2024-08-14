#include <filesystem>
#include <mdbx.h++>

#include "mdbx.h"

// Defined in the implementation file to avoid mdbx includes in the header, in
// accordance with the needs of libbitcoinkernel.

struct MDBXContext {
    mdbx::env::operate_parameters operate_params;
    mdbx::env_managed::create_parameters create_params;

    mdbx::env_managed env;
    mdbx::txn_managed txn;
    mdbx::map_handle map;

    ~MDBXContext()
    {
        txn.abort(); // Why does a double-free happen if txn.abort() is commented out?
        env.close();
    }
};

MDBXWrapper::MDBXWrapper(std::filesystem::path path)
    : DBWrapperBase(DBParams{}),
    m_db_context{std::make_unique<MDBXContext>()}
{
    DBContext().env = mdbx::env_managed(path, DBContext().create_params, DBContext().operate_params);
    DBContext().txn = DBContext().env.start_write();
    DBContext().map = DBContext().txn.create_map(nullptr, mdbx::key_mode::usual, mdbx::value_mode::single);
};

MDBXWrapper::~MDBXWrapper()
{
}
