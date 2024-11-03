
//#include "common.h"
#include "mmap_file.h"
#include "mmap_file_op.h"
#include "file_op.h"
using namespace qiniu;

static const mode_t OPEN_MODE = 0644;
// 内存映射的参数
const static largefile::MMapOption mmap_option = { 10240000, 4096, 4096 };

int open_file(std::string file_name, int open_flag)
{
	int fd = open(file_name.c_str(), open_flag, OPEN_MODE);
	if (fd < 0)
	{
		return -errno;
	}
	return fd;
}
int test01()
{
	const char* file = "./mapfile_test.txt";
	// 打开一个文件,如果没有则创建
	int fd = open_file(file, O_RDWR | O_CREAT | O_LARGEFILE);
	if (fd < 0)
	{
		fprintf(stderr, "open file err. filename: %s, err desc: %s\n",
			file, strerror(-fd));
		return -1;
	}
	largefile::MmapFile* map_file = new largefile::MmapFile(mmap_option, fd);
	bool is_mapped = map_file->map_file(true);
	if (map_file)
	{
		// 测试扩容
		map_file->remap_file();
		// 设置数值
		memset(map_file->get_data(), '9', map_file->get_size());
		// 设置同步
		map_file->sync_file();
		// 解除映射
		map_file->munmap_file();
	}
	else
	{
		fprintf(stderr, "map_file failed\n");
	}


	// 关闭文件描述符
	close(fd);
}
void test02()
{
	const char* file_name = "./file_OP_test.txt";
	largefile::FileOperation* file_OP = new largefile::FileOperation(file_name, O_RDWR | O_LARGEFILE | O_CREAT);
	char buf[10];
	memset(buf, '6', sizeof(buf));

	file_OP->pwrite_file(buf, sizeof(buf), 2);

	memset(buf, 0, sizeof(buf));
	file_OP->pread_file(buf, sizeof(buf), 2);
	printf("%s\n", buf);
	file_OP->close_file();
}
void test03()
{
	const char* file_name = "mmapfileoperation_test.txt";
	largefile::MmapFileOperation* mmfo = new largefile::MmapFileOperation(file_name);
	mmfo->open_file();
	mmfo->mmap_file(mmap_option);
	char buf1[48];
	memset(buf1, '8', sizeof(buf1));
	mmfo->pwrite_file(buf1, sizeof(buf1), 20);
	mmfo->flush_file();
	char buf2[49];
	memset(buf2, 0, sizeof(buf2) - 1);
	mmfo->pread_file(buf2, sizeof(buf2), 20);
	buf2[49] = '\0';
	printf("buf2 : %s\n", buf2);

	mmfo->pwrite_file(buf1, 128, 4000);
	mmfo->munmap_file();

	mmfo->close_file();

}
int main3()
{
	

	return 0;
}
