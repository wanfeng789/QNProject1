#include "mmap_file.h"

static int debug = 1;		// 便于调试信息

qiniu::largefile::MmapFile::MmapFile():
	size_(0), fd_(0), data_(NULL)
{

}

qiniu::largefile::MmapFile::MmapFile(const int fd) :
	size_(0), fd_(fd), data_(NULL)
{

}

qiniu::largefile::MmapFile::MmapFile(const MMapOption& mmap_option, const int fd) :
	size_(0), fd_(fd), data_(NULL)
{
	mmap_file_option.first_mmap_size_ = mmap_option.first_mmap_size_;
	mmap_file_option.max_mmap_size_ = mmap_option.max_mmap_size_;
	mmap_file_option.per_mmap_size_ = mmap_option.per_mmap_size_;


}

qiniu::largefile::MmapFile::~MmapFile()
{
	if (data_)
	{
		if (debug)
		{
			printf("mmap_file destruct, map : fd = %d, size: %d, data"
				": %p\n", fd_, size_, data_);
		}
		// 实现磁盘同步
		msync(data_, size_, MS_SYNC);
		// 释放映射信息
		munmap(data_, size_);
		// 将数据进行归0
		size_ = 0;
		data_ = NULL;
		fd_ = -1;
		mmap_file_option.first_mmap_size_ = 0;
		mmap_file_option.max_mmap_size_ = 0;
		mmap_file_option.per_mmap_size_ = 0;
	}
}

bool qiniu::largefile::MmapFile::sync_file()
{
	if (data_ != NULL && size_ > 0)
	{
		// 使用异步无需等到更新完成在返回
		return msync(data_, size_, MS_ASYNC) == 0;	
	}
	return true;
}

bool qiniu::largefile::MmapFile::map_file(const bool write)
{
	if (fd_ < 0 || mmap_file_option.max_mmap_size_ == 0)
	{
		return false;
	}
	int prot_ = PROT_READ;
	if (write)
	{
		// 加上可写的权限
		prot_ |= PROT_WRITE;
	}
	if (size_ < mmap_file_option.max_mmap_size_)
	{
		size_ = mmap_file_option.first_mmap_size_;
	}
	else
	{
		size_ = mmap_file_option.max_mmap_size_;
	}
	// 调整下文件的大小
	if (ensure_file_size(size_) == false)
	{
		fprintf(stderr, "ensure_file_size failed"
			"in map_file\n");
		return false;
	}

	data_ = mmap(NULL, size_, prot_, MAP_SHARED, fd_, 0);
	
	if (MAP_FAILED == data_)	// 调用失败
	{
		fprintf(stderr, "map failed: %s\n", strerror(errno));
		size_ = 0;
		fd_ = -1;
		data_ = NULL;
		return false;
	}
	if (debug)
	{
		printf("map success! fd: %d, data: %p, size: %d\n", fd_, data_, size_);
	}
	return true;
}

void* qiniu::largefile::MmapFile::get_data() const
{
	return data_;
}

int32_t qiniu::largefile::MmapFile::get_size() const
{
	return size_;
}

bool qiniu::largefile::MmapFile::munmap_file()
{
	if (munmap(data_, size_) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool qiniu::largefile::MmapFile::remap_file()
{
	if (fd_ < 0 || data_ == NULL)
	{
		fprintf(stderr, "mremap not map\n");
		return false;
	}
	if (size_ == mmap_file_option.max_mmap_size_)
	{
		// 已经超过或者等于最大要求的大小了
		fprintf(stderr, "already exceed map_max_size\n");
		return false;
	}
	int new_size = size_ + mmap_file_option.per_mmap_size_;
	if (new_size > mmap_file_option.max_mmap_size_)
	{
		fprintf(stderr, "exceed max_size in mremap\n");
		return false;
	}
	// 重新调整文件大小
	if (ensure_file_size(new_size) == false)
	{
		fprintf(stderr, "ensure_file_size failed"
			"in remap_file\n");
		return false;
	}
	if (debug)
	{
		printf("remap start, now fd: %d, now size: %d, old data: %p\n",
			fd_, size_, data_);
	}
	void* new_map_data = mremap(data_, size_, new_size, MREMAP_MAYMOVE);
	if (new_map_data == MAP_FAILED)
	{
		fprintf(stderr, "mremap failed, fd: %d, new size: %d,"
			"desc err %s", fd_, new_size, strerror(errno));
		return false;
	}

	data_ = new_map_data;
	size_ = new_size;
	return true;
}

bool qiniu::largefile::MmapFile::ensure_file_size(const int32_t size)
{
	struct stat s;

	if (fstat(fd_, &s) < 0)
	{
		fprintf(stderr, "fstat failed: %s", strerror(errno));
		return false;
	}
	// 需要进行扩容操作
	if (s.st_size < size)
	{
		
		if (ftruncate(fd_, size) < 0)
		{
			fprintf(stderr, "ftruncate failed: %s\n", strerror(errno));
			return false;
		}

	}
	return true;
}
