#include <filesystem>
#include <mdbx.h++>

#include "mdbx.h"

MDBXWrapper::MDBXWrapper(std::filesystem::path path)
{
    operate_params = mdbx::env::operate_parameters(1);
    env = mdbx::env_managed(path, operate_params);
};
