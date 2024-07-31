#include <filesystem>
#include <mdbx.h++>

#include "mdbx.h"

MDBXWrapper::MDBXWrapper(std::filesystem::path path)
{
    env = mdbx::env_managed(path, create_params, operate_params);
    txn = env.start_write();
    map = txn.create_map(nullptr, mdbx::key_mode::usual, mdbx::value_mode::single);
};
