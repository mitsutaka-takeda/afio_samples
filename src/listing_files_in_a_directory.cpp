#include <vector>
#include <boost/afio.hpp>

#include "async/future_extension.hpp"

int main(int argc, char *argv[]) {
    if(argc != 2){
        std::cout << "You need to specify a file path at the command line." << std::endl;
        return 1;
    }

    std::vector<char const*> args(argv, argv + argc);

    auto async_file_io_dispatcher
        = boost::afio::make_async_file_io_dispatcher();
    // 指定されてディレクトリを開く。
    auto dir = async_file_io_dispatcher->dir(boost::afio::async_path_op_req(args[1]));
    auto enumerate = async_file_io_dispatcher->enumerate(
        boost::afio::async_enumerate_op_req(
            dir));
    async::then(std::move(enumerate.first),
        [](const auto f){
            for(auto&& entry : f.first){
                std::cout << entry.name() << std::endl;
            }
         }).wait();
    return 0;
}
