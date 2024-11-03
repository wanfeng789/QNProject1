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


int main12(int argc, char* argv[])
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
	

	delete index_handle;
	return 0;
}










