#include <algorithm>
#include <random>
#include <vector>

#include <boost/afio.hpp>
#include <boost/afio/detail/Aligned_Allocator.hpp>
#include <boost/filesystem.hpp>

int main(int argc, char *argv[]) {

    if(argc < 2){
        std::cout << "You need to specify number of files to write.\n";
    }

    const auto number_of_files = std::stoi(argv[1]);

    auto async_file_io_dispatcher(boost::afio::make_async_file_io_dispatcher());
    // create a test directory
    auto test_directory_path = boost::filesystem::temp_directory_path()/boost::filesystem::unique_path();
    auto test_directory = async_file_io_dispatcher->dir(
        boost::afio::async_path_op_req::absolute(
            boost::afio::async_io_op(),
            test_directory_path,
            boost::afio::file_flags::Create));

    // batch create tmp files in the test directory.
    auto file_path_generator = [&]{
        return boost::afio::async_path_op_req::absolute(
            test_directory,
            test_directory_path/boost::filesystem::unique_path(),
            // 注意:
            // boost::afio::file_flags::Write を付けないとbad file descriptorで例外が起きる。
            // terminate called after throwing an instance of 'boost::system::system_error'
            //             what():  Bad file descriptor (9) in '/home/mtakeda/src/cpp/boost_msm_practices/blocks/mitsutaka_takeda/afio_samples/external/boost.afio/include/boost/afio/detail/impl/afio.ipp':dowrite:2781 [Path: '/tmp/fb9f-1208-071d-cfdd/4214-91fd-b8d1-8e8a']: Bad file descriptor
            boost::afio::file_flags::Create | boost::afio::file_flags::Write | boost::afio::file_flags::Append);
    };

    std::vector< boost::afio::async_path_op_req > create_files{};
    std::generate_n(std::back_inserter(create_files),  number_of_files, file_path_generator);

    auto files_created = async_file_io_dispatcher->file(create_files);

    // batch write randowm contents to the files.

    std::default_random_engine generator;
    std::uniform_int_distribution<char> distribution('a','Z');
    auto random_char_generator = [&]{
        return distribution(generator);
    };
    std::vector<char> data;
    std::generate_n(std::back_inserter(data),  1500 * number_of_files, random_char_generator);
    std::vector<boost::asio::mutable_buffer> buffers;
    std::generate_n(std::back_inserter(buffers),
                    number_of_files,
                    [&, i = 0, len = 1500]() mutable {
                        return boost::asio::mutable_buffer(data.data() + (i++)*len, len);
                    });

    auto write_file_generator = [&, i = 0, len = 1500]() mutable {
        const auto t = i++;
        return boost::afio::make_async_data_op_req(
            files_created[t],
            buffers[t],
            0
            );
    };
    std::vector< boost::afio::async_data_op_req<boost::asio::mutable_buffer> > write_files;
    std::generate_n(std::back_inserter(write_files), number_of_files, write_file_generator);
    auto written_files = async_file_io_dispatcher->write(write_files);

    // 注意: syncを呼ばないとファイルに実際に書き込まれる前にプログラムが終了してしまう。
    auto stored = async_file_io_dispatcher->sync(written_files);
    boost::afio::when_all(stored).wait();
    return 0;
}
