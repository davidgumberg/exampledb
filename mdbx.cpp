#include <filesystem>
#include <mdbx.h++>

#include "mdbx.h"

MDBXWrapper::MDBXWrapper(std::filesystem::path path)
{
    env = mdbx::env_managed(path, create_params, operate_params);
};
