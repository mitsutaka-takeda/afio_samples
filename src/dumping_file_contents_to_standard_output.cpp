#include <functional>
#include <vector>

#include <boost/afio.hpp>
#include <boost/afio/detail/Aligned_Allocator.hpp>

#include "async/future_extension.hpp"

// ファイル内容をread後のcompletionハンドラー。第1 & 2引数は、
// AFIO固有のコールバック(std::function<async_file_io_dispatcher_base::completion_t>)の引数。
std::pair<bool, std::shared_ptr<boost::afio::async_io_handle> >
print_file_contents(
    size_t id,
    boost::afio::async_io_op precondition,
    std::shared_ptr<std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> > > buf,
    size_t length
    ) {

    std::cout.write(buf->data(), length);

    // このcompletionハンドラーがopを完了した場合はtrue。
    // 完了されたopへの参照を返す。
    return std::make_pair(true, precondition.get());
}

// Afioの基本的な流れ。
// 1. requestを作成。
// 2. dispatcherにrequestを投げて、async_io_opオブジェクトを作成。
// 3. async_io_opオブジェクトを元に継続(続けて行いたい処理)のrequestを作成。2に戻る。

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
    // async_io_opの対象ハンドルasync_io_handleからdirectory_entryを取得してファイル・サイズを取得。
    auto length(file_open.get()->direntry().st_size());
    auto buffer = std::make_shared<std::vector<char, boost::afio::detail::aligned_allocator<char, 4096, false> > >(
        length);

    // 3 & 2. 「ファイルを開く」操作の後に「ファイルを読む」操作のためのrequest(async_data_op_req)を生成。
    // requestをdispatcherに投げて「ファイルを読む」操作を作成。
    auto file_read(async_file_io_dispatcher->read(
                       boost::afio::make_async_data_op_req(
                           file_open,
                           buffer->data(),
                           length, // ここにファイル・サイズより大きい値を設定すると例外。FATAL EXCEPTION: Failed to read all buffers。
                           0)));

    // 「ファイルを読む」完了後に標準出力にファイルの内容を出力する操作を生成。
    boost::afio::when_all( // when_allで、opの完了を待つfutureを生成できる。
        async_file_io_dispatcher->completion(
            file_read,
            std::make_pair(
                boost::afio::async_op_flags::none,
                std::bind(
                    &print_file_contents,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    buffer,
                    length
                    ))
            )).wait();
    return 0;
}
