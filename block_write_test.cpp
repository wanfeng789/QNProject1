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


int main111(int argc, char* argv[])
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
		//delete mainblock;
		//index_handle->remove(block_id);
		delete index_handle;
		exit(-2);
	}
	// 2.写入文件到主块文件
	std::stringstream tmp_stream;
	tmp_stream << "." << largefile::MAINBLOCK_DIR_PERFIX << block_id;
	// 生成主块文件路径
	tmp_stream >> mainblock_path;
	// 生成主块文件 
	largefile::FileOperation* mainblock = new largefile::FileOperation(mainblock_path, O_RDWR | O_CREAT | O_LARGEFILE);
	// 设置主块文件大小
	ret = mainblock->ftruncate_file(main_blocksize);

	char buf[4096];
	memset(buf, '6', sizeof(buf));
	int32_t data_offset = index_handle->get_block_data_offset();
	uint32_t file_no = index_handle->block_info()->seq_no_;
	ret = mainblock->pwrite_file(buf, sizeof(buf), data_offset);
	if (ret != qiniu::largefile::TFS_SUCCESS)
	{
		fprintf(stderr, "write main block err, reason:%s\n", strerror(ret));
		delete mainblock;
		delete index_handle;
		exit(-3);
	}

	// 索引文件中写入Metainfo
	qiniu::largefile::MetaInfo meta;
	meta.set_file_id(file_no);
	meta.set_offset(data_offset);
	meta.set_size(sizeof(buf));

	ret = index_handle->write_segment_meta(meta.get_key(), meta);
	if (ret == largefile::TFS_SUCCESS)
	{
		// 1. 更新索引头部信息
		index_handle->commit_block_data_offset(sizeof(buf));
		// 2. 更新块信息
		index_handle->update_block_info(largefile::C_OPER_INSERT, sizeof(buf));
		
		// 更新到文件里
		ret = index_handle->flush();
		if (ret != largefile::TFS_SUCCESS)
		{
			fprintf(stderr, "flush mainblock:%u err\n", block_id);

		}

	}
	else
	{
		fprintf(stderr, "write segment meta err\n");
	}
	if (ret != largefile::TFS_SUCCESS)
	{
		fprintf(stderr, "write to mainblock %d faild\n", block_id);
	}
	else
	{
		printf("write to mainblock %d success\n", block_id);
	}



	mainblock->close_file();
	delete mainblock;
	delete index_handle;
	return 0;
}










