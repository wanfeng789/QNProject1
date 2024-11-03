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


int main11(int argc, char* argv[])
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
	// 1.生成主块文件
	std::stringstream tmp_stream;
	tmp_stream << "." << largefile::MAINBLOCK_DIR_PERFIX  << block_id;
	// 生成主块文件路径
	tmp_stream >> mainblock_path;
	// 生成主块文件 
	largefile::FileOperation* mainblock = new largefile::FileOperation(mainblock_path, O_RDWR | O_CREAT | O_LARGEFILE);
	// 设置主块文件大小
	ret = mainblock->ftruncate_file(main_blocksize);
	if (ret != 0)
	{
		fprintf(stderr, "create mainblock err path: %s, reason: %s\n", mainblock_path.c_str(), strerror(ret));
		delete mainblock;
		exit(-2);
	}
	// 创建索引文件
	largefile::IndexHandle* index_handle = new largefile::IndexHandle(".", block_id);

	if (debug)
	{
		printf("init index....\n");
	}
	ret = index_handle->create(block_id, bucket_size, mmap_option);
	if (ret != largefile::TFS_SUCCESS)
	{
		fprintf(stderr, "create index id err, %d\n", block_id);
		delete mainblock;
		index_handle->remove(block_id);
		delete index_handle;
		exit(-3);
	}
	// 其他操作
	mainblock->close_file();
	index_handle->flush();

	delete mainblock;
	delete index_handle;
	return 0;
}





