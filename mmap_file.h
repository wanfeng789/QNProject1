
#ifndef _LARGE_MMAP_FILE
#define _LARGE_MMAP_FILE

#include "common.h"

namespace qiniu
{
	namespace largefile
	{
		class MmapFile
		{
		public:
			MmapFile();		// 默认构造函数
			explicit MmapFile(const int fd);
			MmapFile(const MMapOption& mmap_option, const int fd);

			~MmapFile();

			bool sync_file();		// 同步文件
			// 将文件映射到内存, 同时设置访问权限
			bool map_file(const bool write = false);
			void* get_data()const;// 获取映射到内存数据的首地址
			int32_t get_size()const;// 获取映射到数据的大小

			bool munmap_file();		// 解除映射
			bool remap_file();		// 重新执行映射 mremap()
		private:
			bool ensure_file_size(const int32_t size);
		private:
			int32_t size_;
			int fd_;
			void* data_;
			MMapOption mmap_file_option;
		};
	}
}



#endif // 



