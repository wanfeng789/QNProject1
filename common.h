
#ifndef _COMMON_H_
#define _COMOON_H_

#include <string>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <cstdio>
#include <cinttypes>
#include <sys/mman.h>
#include <cstring>
#include <sys/stat.h>
#include <errno.h>
#include <cassert>
namespace qiniu
{
	namespace largefile
	{
		// 读或者写的长度少于要求的长度
		const int32_t EXIT_DISK_OPER_INCOMPLETE = -8012;
		// 索引文件已经被创建或者加载
		const int32_t EXIT_INDEX_ALREADY_LOADED_ERROR = -8013;
		const int32_t TFS_SUCCESS = 0;
		const int32_t TFS_ERROR = -1;
		// 索引文件已经存在
		const int32_t EXIT_META_UNEXPECT_FOUND_ERROR = -8014;
		// 索引文件已经损坏
		const int32_t EXIT_INDEX_CORRUPT_ERROR = -8015;	
		const int32_t EXIT_BLOCK_CONFLICT_ERROR = -8016;
		const int32_t EXIT_BUCKET_CONFIGURE_ERROR = -8017;
		const int32_t EXIT_META_NOT_FOUND_ERROR = -8018;
		const int32_t EXIT_BLOCKID_ZERO_ERROR = -8019;

		enum OperType
		{
			C_OPER_INSERT = 1,
			C_OPER_DELETE
		};
		// 基本的配置目录
		// 主块的路径
		static const std::string MAINBLOCK_DIR_PERFIX = "/mainblock/";
		// 索引的路径
		static std::string INDEX_DIR_PERFIX = "/index/";


		static const mode_t DIR_MODE = 0755;	// 设置目录的访问权限

		struct MMapOption
		{
			int32_t max_mmap_size_;		// 3MB
			int32_t first_mmap_size_;	// 4KB
			int32_t per_mmap_size_;		// 4KB
		};
		// 块的信息
		struct BlockInfo
		{
			uint32_t block_id_;	// 块编号
			int32_t version_;	// 块当前编号
			int32_t file_count_;	// 当前已保存文件总数
			int32_t size_;		// 当前已保存文件数据总大小
			int32_t del_file_count_;	// 已删除的文件数量
			int32_t del_size_;	// 已删除的文件数据总大小
			uint32_t seq_no_;	// 下一个可分配的文件编号
			BlockInfo()
			{
				memset(this, 0, sizeof(BlockInfo));
			}
			// 重载 == 符号
			bool operator== (BlockInfo& tfs)
			{
				return this->block_id_ == tfs.block_id_
					&& this->version_ == tfs.version_
					&& this->file_count_ == tfs.file_count_
					&& this->size_ == tfs.size_
					&& this->del_file_count_ == tfs.del_file_count_
					&& this->del_size_ == tfs.del_size_
					&& this->seq_no_ == tfs.seq_no_;
			}
		};

		struct MetaInfo
		{
		public:
			MetaInfo()
			{
				init();
			}
			MetaInfo(uint64_t fileid, int32_t offset, int32_t size, int32_t next_meta_offset)
			{
				fileid_ = fileid;
				location_.inner_offset_ = offset;
				location_.size_ = size;
				next_meta_offset_ = next_meta_offset;
			}
			// 重载拷贝构造函数
			MetaInfo(MetaInfo& me_In)
			{
				memcpy(this, &me_In, sizeof(MetaInfo));
			}
			// 重载 = 操作
			MetaInfo& operator=(MetaInfo& me_In)
			{
				
				if (this == &me_In)
				{
					return *this;
				}
				fileid_ = me_In.fileid_;
				location_.inner_offset_ = me_In.location_.inner_offset_;
				location_.size_ = me_In.location_.size_;
				next_meta_offset_ = me_In.next_meta_offset_;
			}
			// 克隆
			MetaInfo& clone(MetaInfo& me_In)
			{
				assert(this != &me_In);
				fileid_ = me_In.fileid_;
				location_.inner_offset_ = me_In.location_.inner_offset_;
				location_.size_ = me_In.location_.size_;
				next_meta_offset_ = me_In.next_meta_offset_;
				return *this;
			}
			// 重载 == 符号
			bool operator ==(MetaInfo& me_In)
			{
				return fileid_ == me_In.fileid_
					&& location_.inner_offset_ == me_In.location_.inner_offset_
					&& location_.size_ == me_In.location_.size_
					&& next_meta_offset_ == me_In.next_meta_offset_;
			}
		public:
			// 获取文件编号
			uint64_t get_key()
			{
				return fileid_;
			}
			// 设置文件编号
			void set_key(uint64_t key)
			{
				fileid_ = key;
			}
			// 获取文件编号
			uint64_t get_file_id()
			{
				return fileid_;
			}
			// 设置文件编号
			void set_file_id(uint64_t file_id)
			{
				fileid_ = file_id;
			}
			
			int32_t get_offset()
			{
				return location_.inner_offset_;
			}
			void set_offset(int32_t offset)
			{
				location_.inner_offset_ = offset;
			}
			int32_t get_size()
			{
				return location_.size_;
			}
			void set_size(int32_t size)
			{
				location_.size_ = size;
			}
			int32_t get_next_meta_offset()
			{
				return next_meta_offset_;
			}
			void set_next_meta_offset(int32_t next_offset)
			{
				next_meta_offset_ = next_offset;
			}
		private:
			uint64_t fileid_;		// 文件编号
			struct
			{
				int32_t inner_offset_;	// 文件在块内部的偏移量
				int32_t size_;			// 文件大小
			}location_;
			// 当前哈希链下一个节点在索引文件中的偏移量
			int32_t next_meta_offset_;
		private:
			void init()
			{
				fileid_ = 0;
				location_.inner_offset_ = 0;
				location_.size_ = 0;
				next_meta_offset_ = 0;
			}
		};

	}
}

#endif
