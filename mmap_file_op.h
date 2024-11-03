
#ifndef _MMAP_FILE_OP_
#define _MMAP_FILE_OP_
#include "mmap_file.h"
#include "file_op.h"
namespace qiniu
{
	namespace largefile
	{
		class MmapFileOperation :
			public FileOperation
		{
		public:
			MmapFileOperation(std::string file_name, const int file_flags = O_RDWR | O_LARGEFILE | O_CREAT);

			~MmapFileOperation();

			// 写数据
			int pread_file(char* buf, int32_t size, int64_t offset);
			// 读数据
			int pwrite_file(const char* buf, int32_t size, int64_t offset);
			
			// 文件映射
			int mmap_file(const MMapOption& mmapoption);
			// 取消映射
			int munmap_file();
			// 获取映射内存的首地址
			void* get_map_data()const;
			// 同步文件到磁盘
			int flush_file();
		private:
			MmapFile* map_file_;
			bool ismapped_;
		};
	}
}

#endif