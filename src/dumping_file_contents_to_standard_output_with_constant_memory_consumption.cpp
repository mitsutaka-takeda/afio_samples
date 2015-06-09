#include <functional>
#include <vector>

#include <boost/afio.hpp>
#include <boost/afio/detail/Aligned_Allocator.hpp>

#include "async/future_extension.hpp"

std::pair<bool, std::shared_ptr<boost::afio::async_io_handle> >
print_file_contents(
    size_t id,
    boost::afio::async_io_op precondition,
    std::shared_ptr<std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> > > buf,
    size_t file_size,
    std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> >::size_type max_buffer_size,
    std::shared_ptr<std::promise<void> > finisher
    );

void
read_file(
    boost::afio::async_io_op precondition,
    std::shared_ptr<std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> > > buf,
    size_t file_size,
    std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> >::size_type max_buffer_size,
    std::shared_ptr<std::promise<void> > finisher
    ){

    const auto remainder = file_size - precondition->read_count();
    buf->resize(std::min(static_cast<decltype(remainder)>(max_buffer_size), remainder));
    auto file_read(
        precondition.parent->read(
            boost::afio::make_async_data_op_req(
                precondition,
                buf->data(),
                buf->size(),
                precondition->read_count())));
    precondition.parent->completion(
        file_read,
        std::make_pair(
            boost::afio::async_op_flags::none,
            std::bind(
                &print_file_contents,
                std::placeholders::_1,
                std::placeholders::_2,
                buf,
                file_size,
                max_buffer_size,
                std::move(finisher)
                )));
}

// ファイル内容をread後のcompletionハンドラー。第1 & 2引数は、
// AFIO固有のコールバック(std::function<async_file_io_dispatcher_base::completion_t>)の引数。
std::pair<bool, std::shared_ptr<boost::afio::async_io_handle> >
print_file_contents(
    size_t id,
    boost::afio::async_io_op precondition,
    std::shared_ptr<std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> > > buf,
    size_t file_size,
    std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> >::size_type max_buffer_size,
    std::shared_ptr<std::promise<void> > finisher
    ) {
    // async_io_handle::read_countで読み込み位置のoffsetが取得できる。
    const auto offset = precondition->read_count();
    const auto remainder = file_size - offset;

    std::cout.write(buf->data(), buf->size());

    if(remainder != 0) {
        read_file(precondition, buf,  file_size, max_buffer_size, std::move(finisher));
    } else {
        finisher->set_value();
    }

    // このcompletionハンドラーがopを完了した場合はtrue。完了されたopへの参照を返す。
    // falseを返すとstackに処理されていない操作があると警告が出る。
    // WARNING: ~async_file_dispatcher_base() detects stuck async_io_op in total of 8 extant ops
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

    // 1 & 2 : path requestを作成 & dispatcherに投げて「ファイルを開く」asyn_io_opを作成。
    auto file_open(async_file_io_dispatcher->file(
                       boost::afio::async_path_op_req::absolute(
                           boost::afio::async_io_op(),
                           args[1])));
    // このファイル・サイズ取得もコール・バックの中でしないとダメな気がする。
    const auto file_size(file_open->direntry().st_size());

    // page_sizeからbuffer_sizeを計算。vector::frontがconstexprじゃない。。。
    const auto max_buffer_size = boost::afio::utils::page_sizes().front();// 4096 on Ubuntu.
    auto buffer = std::make_shared<std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> > >();
    buffer->reserve(max_buffer_size+1);

    // 処理の終りを通知するfuture/promise。
    auto finisher = std::make_shared<std::promise<void>>();
    auto finished(finisher->get_future());

    read_file(file_open, buffer,  file_size, max_buffer_size, std::move(finisher));

    finished.wait();

    return 0;
}
