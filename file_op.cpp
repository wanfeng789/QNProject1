#include "file_op.h"

qiniu::largefile::FileOperation::FileOperation(std::string file_name, const int file_flags):
	fd_(-1), open_flags_(file_flags)
{
	// strdup 开辟新内存复制
	file_name_ = strdup(file_name.c_str());
}

qiniu::largefile::FileOperation::~FileOperation()
{
	if (fd_ > 0)
	{
		::close(fd_);
		fd_ = -1;
	}
	if (file_name_ != NULL)
	{
		free(file_name_);
		file_name_ = NULL;
	}
}

int qiniu::largefile::FileOperation::open_file()
{
	// 如果存在, 先关闭在打开
	if (fd_ > 0)
	{
		::close(fd_);
		fd_ = -1;
	}
	fd_ = ::open(file_name_, open_flags_, OPEN_MODE);
	if (fd_ < 0)
	{
		return errno;
	}
	return fd_;
}

void qiniu::largefile::FileOperation::close_file()
{
	if (fd_ < 0)
	{
		return;
	}
	::close(fd_);
	fd_ = -1;
}

int qiniu::largefile::FileOperation::flush_file()
{
	// 文件打开属性如果为 同步 
	if (open_flags_ & O_SYNC)
	{
		return 0;
	}
	int fd = check_file();
	if (fd < 0)
	{
		return -1;
	}
	return fsync(fd_);

	return 0;
}

int qiniu::largefile::FileOperation::unlink_file()
{
	close_file();
	return unlink(file_name_);
}

int qiniu::largefile::FileOperation::pread_file(char* buf, int32_t nbytes, const int64_t offset)
{
	// 循环读取数据
	int32_t left_bytes = nbytes;
	int64_t right_offset = offset;
	int32_t read_len = 0;
	char* right_p = buf;

	int i = 0;		// 记录读取磁盘的次数
	while (true)
	{
		++i;
		if (i >= MAX_DISK_TIMES)
		{
			break;
		}
		if (check_file() < 0)
		{
			return -errno;
		}
		read_len = pread64(fd_, right_p, left_bytes, right_offset);
		if (read_len < 0)
		{
			read_len = -errno;
			// 没有读到数据, 继续读
			if (-read_len == EINTR || -read_len == EAGAIN)
			{
				continue;
			}
			// 读数据失败
			else if (-read_len == EBADF)
			{
				fd_ = -1;
				return read_len;
			}
		}
		else if(read_len == 0)
		{
			break;
		}
		left_bytes -= read_len;
		right_p += read_len;
		right_offset += offset;
	}
	// 进行读数据有问题
	if (left_bytes != 0)
	{
		return EXIT_DISK_OPER_INCOMPLETE;
	}
	return TFS_SUCCESS;
}

int qiniu::largefile::FileOperation::pwrite_file(const char* buf, const int32_t nbytes, const int64_t offset)
{
	// 循环写入数据
	int32_t left_bytes = nbytes;
	int64_t right_offset = offset;
	int32_t write_len = 0;
	 char* right_p = (char*)buf;

	int i = 0;		// 记录写入磁盘的次数
	while (true)
	{
		++i;
		if (i >= MAX_DISK_TIMES)
		{
			break;
		}
		if (check_file() < 0)
		{
			return -errno;
		}
		write_len = pwrite64(fd_, right_p, left_bytes, right_offset);
		if (write_len < 0)
		{
			write_len = -errno;
			// 没有读到数据, 继续读
			if (-write_len == EINTR || -write_len == EAGAIN)
			{
				continue;
			}
			// 读数据失败
			else if (-write_len == EBADF)
			{
				fd_ = -1;
				return write_len;
			}
		}
		else if (write_len == 0)
		{
			break;
		}
		left_bytes -= write_len;
		right_p += write_len;
		right_offset += offset;
	}
	// 进行读数据有问题
	if (left_bytes != 0)
	{
		return EXIT_DISK_OPER_INCOMPLETE;
	}
	return TFS_SUCCESS;
}

int qiniu::largefile::FileOperation::write_file(const char* buf, const int32_t nbytes)
{
	// 循环写入数据
	int32_t left_bytes = nbytes;
	int32_t write_len = 0;
	char* right_p = (char*)buf;

	int i = 0;		// 记录写入磁盘的次数
	while (true)
	{
		++i;
		if (i >= MAX_DISK_TIMES)
		{
			break;
		}
		if (check_file() < 0)
		{
			return -errno;
		}
		write_len = write(fd_, right_p, left_bytes);
		if (write_len < 0)
		{
			write_len = -errno;
			// 没有读到数据, 继续读
			if (-write_len == EINTR || -write_len == EAGAIN)
			{
				continue;
			}
			// 读数据失败
			else if (-write_len == EBADF)
			{
				fd_ = -1;
				return write_len;
			}
		}
		else if (write_len == 0)
		{
			break;
		}
		left_bytes -= write_len;
		right_p += write_len;
	}
	// 进行读数据有问题
	if (left_bytes != 0)
	{
		return EXIT_DISK_OPER_INCOMPLETE;
	}
	return TFS_SUCCESS;
}

int64_t qiniu::largefile::FileOperation::get_file_size()
{
	int fd = check_file();
	if (fd < 0)
	{
		return -1;
	}
	// 获取文件大小
	struct stat s_buf;
	if (fstat(fd_, &s_buf) != 0)
	{
		return -1;
	}
	return s_buf.st_size;
}

int qiniu::largefile::FileOperation::ftruncate_file(const int64_t length)
{
	int fd = check_file();
	if (fd < 0)
	{
		return -1;
	}

	return ftruncate(fd_, length);
}

int qiniu::largefile::FileOperation::seek_file(const int64_t offset)
{
	int fd = check_file();
	if (fd < 0)
	{
		return -1;
	}
	// 相对于头部偏移
	return lseek(fd_, offset, SEEK_SET);
}

int qiniu::largefile::FileOperation::get_fd() const
{
	return fd_;
}

int qiniu::largefile::FileOperation::check_file()
{
	if (fd_ < 0)
	{
		fd_ = open_file();
	}
	return fd_;
}
