
#ifndef _FILE_OP_
#define _FILE_OP_
#include "mmap_file.h"

namespace qiniu
{
	namespace largefile
	{
		class FileOperation
		{
		public:
			FileOperation(std::string file_name, const int file_flags = O_RDWR | O_LARGEFILE);
			~FileOperation();

			int open_file();
			void close_file();
			// 将文件立即写入磁盘
			int flush_file();
			int unlink_file();	// 删除文件
			virtual int pread_file(char* buf, int32_t nbytes, const int64_t offset);
			virtual int pwrite_file(const char* buf, const int32_t nbytes, const int64_t offset);
			int write_file(const char* buf, const int32_t nbytes);

			int64_t get_file_size();
			int ftruncate_file(const int64_t length);
			int seek_file(const int64_t offset);
			int get_fd()const;


		protected:
			int fd_;
			int open_flags_;
			char* file_name_;
		protected:
			// 检查文件是否打开
			int check_file();
		protected:
			static const mode_t OPEN_MODE = 0644;
			// 打开磁盘读写的最大数量
			static const int MAX_DISK_TIMES = 5;
		};
	}

}





#endif