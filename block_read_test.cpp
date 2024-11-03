#include "file_op.h"
#include "index_handle.h"
#include <sstream>

static int debug = 1;
using namespace qiniu;
// 内存映射参数
const static largefile::MMapOption mmap_option = { 1024000, 4096, 4096 };
// 主块文件大小
const static uint32_t main_blocksize = 1024 * 1024 * 64;
// 哈希桶的大小
const static uint32_t bucket_size = 1000;
static uint32_t block_id = 1;


int main(int argc, char* argv[])
{
	std::string mainblock_path;
	std::string index_path;
	int32_t ret = largefile::TFS_SUCCESS;
	std::cout << "请输入块ID" << std::endl;
	std::cin >> block_id;
	if (block_id < 1)
	{
		std::cout << "block_id err" << std::endl;
		exit(-1);
	}

	// 加载索引文件
	largefile::IndexHandle* index_handle = new largefile::IndexHandle(".", block_id);

	if (debug)
	{
		printf("load index....\n");
	}
	ret = index_handle->load(block_id, bucket_size, mmap_option);
	if (ret != largefile::TFS_SUCCESS)
	{
		fprintf(stderr, "load index id err, %d\n", block_id);
		delete index_handle;
		exit(-2);
	}

	//2.读取文件的metainfo
	uint64_t file_id = 0;
	std::cout << "请输入文件ID" << std::endl;
	std::cin >> file_id;
	if (file_id < 1)
	{
		std::cout << "file_id err" << std::endl;
		exit(-2);
	}
	largefile::MetaInfo meta;
	ret = index_handle->read_segment_meta(file_id, meta);
	if (ret != largefile::TFS_SUCCESS)
	{
		fprintf(stderr, "read_segment_meta err, file_id: %u\n", file_id);
		exit(-3);
	}
	// 3.根据meta读文件
	std::stringstream tmp_stream;
	tmp_stream << "." << largefile::MAINBLOCK_DIR_PERFIX << block_id;
	// 生成主块文件路径
	tmp_stream >> mainblock_path;
	// 生成主块文件 
	largefile::FileOperation* mainblock = new largefile::FileOperation(mainblock_path, O_RDWR);
	
	char* buf = (char*)new char[meta.get_size() + 1];
	
	ret = mainblock->pread_file(buf, meta.get_size(), meta.get_offset());
	if (ret != qiniu::largefile::TFS_SUCCESS)
	{
		fprintf(stderr, "read main block err, reason:%s\n", strerror(ret));
		delete mainblock;
		delete index_handle;
		exit(-3);
	}
	buf[meta.get_size()] = '\0';
	printf("read size: %d, buf: %s\n",meta.get_size(), buf);


	mainblock->close_file();
	delete mainblock;
	delete index_handle;
	return 0;
}










