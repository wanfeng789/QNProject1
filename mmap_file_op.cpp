#include "mmap_file_op.h"

static int debug = 1;

qiniu::largefile::MmapFileOperation::MmapFileOperation(std::string file_name, const int file_flags):
	FileOperation(file_name, file_flags), map_file_(NULL), ismapped_(false)
{

}

qiniu::largefile::MmapFileOperation::~MmapFileOperation()
{
	if (map_file_)
	{
		delete map_file_;
		map_file_ = NULL;
	}

}

int qiniu::largefile::MmapFileOperation::pread_file(char* buf, int32_t size, int64_t offset)
{
	//1. 文件已经映射
	
	if (ismapped_ && (size + offset) > map_file_->get_size())
	{
		if (debug)
		{
			printf("MmapFileOperation::pread_file size: %d, offset: %'__PRI64_PREFIX'd, map_file_size : %d\n",
				size, offset, map_file_->get_size());
		}
		// 先进行扩容
		map_file_->remap_file();
	}
	if (ismapped_ && (size + offset) <= map_file_->get_size())
	{
		memcpy(buf, map_file_->get_data() + offset, size);
		return TFS_SUCCESS;
	}
	// 文件没有映射或者映射不全 取父类中读
	return FileOperation::pread_file(buf, size, offset);
}

int qiniu::largefile::MmapFileOperation::pwrite_file(const char* buf, int32_t size, int64_t offset)
{
	//1. 文件已经映射

	if (ismapped_ && (size + offset) > map_file_->get_size())
	{
		if (debug)
		{
			printf("MmapFileOperation::pwrite_file size: %d, offset: %'__PRI64_PREFIX'd, map_file_size : %d\n",
				size, offset, map_file_->get_size());
		}
		// 先进行扩容
		map_file_->remap_file();
	}
	if (ismapped_ && (size + offset) <= map_file_->get_size())
	{
		memcpy(map_file_->get_data() + offset, buf, size);
		return TFS_SUCCESS;
	}
	// 文件没有映射或者映射不全 取父类中写
	return FileOperation::pwrite_file(buf, size, offset);

}

int qiniu::largefile::MmapFileOperation::mmap_file(const MMapOption& mmapoption)
{
	if (mmapoption.max_mmap_size_ <= 0 || mmapoption.max_mmap_size_ < mmapoption.first_mmap_size_)
	{
		return TFS_ERROR;
	}
	// 获得文件描述符
	int fd = check_file();
	if (fd < 0)
	{
		fprintf(stderr, "MmapFileOperation::mmap_file check_file err\n");
		return TFS_ERROR;
	}
	// 进行文件映射
	if (ismapped_ == false)
	{
		if (map_file_)
		{
			delete(map_file_);
		}
		// 获得文件映射对象
		map_file_ = new MmapFile(mmapoption, fd);
		ismapped_ = map_file_->map_file(true);

		if (ismapped_)
		{
			return TFS_SUCCESS;
		}
		else
		{
			return TFS_ERROR;
		}
	}
	
}

int qiniu::largefile::MmapFileOperation::munmap_file()
{
	if (ismapped_ && map_file_ != NULL)
	{
		delete map_file_;
		ismapped_ = false;
	}
	return TFS_SUCCESS;
}

void* qiniu::largefile::MmapFileOperation::get_map_data() const
{
	if (ismapped_)
	{
		return map_file_->get_data();
	}
	return NULL;
}

int qiniu::largefile::MmapFileOperation::flush_file()
{
	// 将数据同步到内存映射区
	if (ismapped_)
	{
		if (map_file_->sync_file())
		{
			return TFS_SUCCESS;
		}
		else
		{
			return TFS_ERROR;
		}
	}
	else
	{
		return FileOperation::flush_file();
	}

}
