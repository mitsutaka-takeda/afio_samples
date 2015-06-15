#include <functional>
#include <vector>

#include <boost/afio.hpp>
#include <boost/afio/detail/Aligned_Allocator.hpp>

#include "async/future_extension.hpp"

std::vector<boost::asio::mutable_buffer>
make_buffers(
    std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> >& memory,
    size_t remainder // ファイルから読み込む残りサイズ
    ){
    const auto buffers_total_size = std::min(memory.size() - 1, remainder);
    const auto number_of_buffers = (buffers_total_size + 4095)/4096;
    std::vector<boost::asio::mutable_buffer> ret;
    ret.reserve(number_of_buffers);

    auto r = buffers_total_size;
    for(auto i = 0; i != number_of_buffers; r -= 4096, ++i) {
        ret.emplace_back(memory.data() + i*4096, std::min(r, static_cast<decltype(r)>(4096)));
    }

    return ret;
}

std::pair<bool, std::shared_ptr<boost::afio::async_io_handle> >
print_file_contents(
    size_t id,
    boost::afio::async_io_op precondition,
    // buffer用のメモリ
    std::shared_ptr<std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> > > memory,
    std::vector<boost::asio::mutable_buffer> buffers, // ファイルから読み出したデータ。
    size_t file_size,
    std::shared_ptr<std::promise<void> > finisher
    );

template <typename T> struct TD;

void
read_file(
    boost::afio::async_io_op precondition,
    // buffer用のメモリ
    std::shared_ptr<std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> > > memory,
    size_t file_size,
    std::shared_ptr<std::promise<void> > finisher
    )
{
    const auto remainder = file_size - precondition->read_count();
    auto buffers = make_buffers(*memory, remainder);

    auto file_read(
        precondition.parent->read(
            // make_async_data_op_reqoの戻り値の型。
            // boost::afio::v1_std_boost_boost::async_data_op_req<std::vector<boost::asio::mutable_buffer> >
            boost::afio::make_async_data_op_req(
                precondition,
                buffers,
                precondition->read_count())));

    precondition.parent->completion(
        file_read,
        std::make_pair(
            boost::afio::async_op_flags::none,
            std::bind(
                &print_file_contents,
                std::placeholders::_1,
                std::placeholders::_2,
                memory,
                buffers,
                file_size,
                std::move(finisher)
                )));
}

std::pair<bool, std::shared_ptr<boost::afio::async_io_handle> >
print_file_contents(
    size_t id,
    boost::afio::async_io_op precondition,
    std::shared_ptr<std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> > > memory,
    std::vector<boost::asio::mutable_buffer> buffers, // ファイルから読み出したデータ。
    size_t file_size,
    std::shared_ptr<std::promise<void> > finisher
    ) {

    const auto offset = precondition->read_count();
    const auto remainder = file_size - offset;

    for(const auto& buffer : buffers) {
        std::cout.write(boost::asio::buffer_cast<char const*>(buffer),
                        boost::asio::buffer_size(buffer));
    }

    if(remainder != 0) {
        read_file(precondition, memory, file_size, std::move(finisher));
    } else {
        finisher->set_value();
    }

    return std::make_pair(true, precondition.get());
}

int main(int argc, char *argv[])
{
    if(argc != 2){
        std::cout << "You need to specify a file path at the command line." << std::endl;
        return 1;
    }

    std::vector<char const*> args(argv, argv + argc);

    auto async_file_io_dispatcher(boost::afio::make_async_file_io_dispatcher());
    auto file_open(async_file_io_dispatcher->file(
                       boost::afio::async_path_op_req::absolute(
                           boost::afio::async_io_op(),
                           args[1])));

    const auto number_of_buffers = 10;
    // このファイル・サイズ取得もコール・バックの中でしないとダメな気がする。
    const auto file_size(file_open->direntry().st_size());
    const auto max_buffer_size = boost::afio::utils::page_sizes().front();// 4096 on Ubuntu.

    auto memory = std::make_shared<std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> > >(max_buffer_size*number_of_buffers + 1);

    // 処理の終りを通知するfuture/promise。
    auto finisher = std::make_shared<std::promise<void>>();
    auto finished(finisher->get_future());

    read_file(file_open, memory,  file_size, std::move(finisher));

    finished.wait();

    return 0;
}
