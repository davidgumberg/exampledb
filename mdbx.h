#include <filesystem>
#include <mdbx.h>
#include <mdbx.h++>

class MDBXWrapper {
public:
    MDBXWrapper(std::filesystem::path path);
private:
    mdbx::env_managed env;
    mdbx::env::operate_parameters operate_params;
    mdbx::env_managed::create_parameters create_params;
    mdbx::txn_managed txn;
};
