
#ifndef INDEX_HANDLE_H
#define INDEX_HANDLE_H

#include "mmap_file_op.h"


namespace qiniu
{
	namespace largefile
	{
		// 块索引
		struct IndexHeader
		{
		public:
			IndexHeader()
			{
				memset(this, 0, sizeof(IndexHeader));
			}
			BlockInfo block_info_;	// 块信息
			int32_t bucket_size_;	// 哈希桶大小
			int32_t data_file_offset_;	//未使用数据起始偏移
			int32_t index_file_size_;	// 索引文件的当前偏移
			int32_t free_head_offset_;	// 可重用的链表节点

		};
		// 文件处理
		class IndexHandle
		{
		public:
			IndexHandle(const std:: string& base_path, uint32_t main_block_id);
			~IndexHandle();
			int create(const uint32_t logic_block_id,
				const int32_t bucket_size, const MMapOption map_option);
			int load(const uint32_t logic_block_id,
				const int32_t bucket_size, const MMapOption map_option);
			// 解除文件映射并且删除文件
			int remove(const uint32_t logic_block_id);
			int flush();

			int write_segment_meta(const uint64_t key, MetaInfo& meta);
			int read_segment_meta(const uint64_t key, MetaInfo& meta);

			int32_t hash_find(const uint64_t key, int32_t& current_offset, int32_t& previous_offset);
			int32_t hash_insert(const uint64_t key, int32_t previous_offset, MetaInfo& meta);
			
			int32_t* bucket_slot()
			{
				return reinterpret_cast<int32_t*>(reinterpret_cast<char*>(file_op_->get_map_data()) + sizeof(IndexHeader));
			}
			IndexHeader* index_header()	// 索引文件起始地址
			{
				return reinterpret_cast<IndexHeader*>(file_op_->get_map_data());
			}
			int update_block_info(const OperType oper_type, const uint32_t modify_size);
			BlockInfo* block_info()
			{
				return reinterpret_cast<BlockInfo*>(file_op_->get_map_data());
			}
			int32_t bucket_size()
			{
				return reinterpret_cast<IndexHeader*>(file_op_->get_map_data())->bucket_size_;
			}
			int32_t get_block_data_offset()
			{
				return reinterpret_cast<IndexHeader*>(file_op_->get_map_data())->data_file_offset_;
			}
			void commit_block_data_offset(const int file_size)
			{
				reinterpret_cast<IndexHeader*>(file_op_->get_map_data())->data_file_offset_ += file_size;
			}
		private:
			bool hash_compare(const uint64_t left_key, const uint64_t right_key)
			{
				return left_key == right_key;
			}
			MmapFileOperation* file_op_;
			bool is_load_;		// 是否被加载
		};
	}
}




#endif

