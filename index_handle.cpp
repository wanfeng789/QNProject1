#include "index_handle.h"

static int debug = 1;

qiniu::largefile::IndexHandle::IndexHandle(const std::string& base_path, uint32_t main_block_id)
{
	// base_path 基础路径
	std::stringstream tmp_stream;
	// 拼接成完整的文件路径
	tmp_stream << base_path << INDEX_DIR_PERFIX << main_block_id;
	// 得到拼接的索引文件的路径
	std::string index_path;
	tmp_stream >> index_path;

	// 创建文件处理
	file_op_ = new MmapFileOperation(index_path, O_RDWR | O_CREAT | O_LARGEFILE);
	is_load_ = false;
}

qiniu::largefile::IndexHandle::~IndexHandle()
{
	if (file_op_)
	{
		delete file_op_;
		file_op_ = NULL;
	}

}

int qiniu::largefile::IndexHandle::create(const uint32_t logic_block_id, const int32_t bucket_size, const MMapOption map_option)
{
	int ret = TFS_SUCCESS;
	if (debug)
	{
		printf("create index, block id: %u, size: %u\n", logic_block_id, bucket_size);
	}
	if (is_load_)
	{
		return EXIT_INDEX_ALREADY_LOADED_ERROR;
	}
	// 获取文件大小
	int file_size = file_op_->get_file_size();
	if (file_size < 0)
	{
		// 获取失败
		return TFS_ERROR;
	}
	else if(file_size ==0 )// 此时文件不存在
	{
		IndexHeader i_header;
		
		i_header.block_info_.block_id_ = logic_block_id;// 块id	
		i_header.block_info_.seq_no_ = 1;	// 下一个可分配的文件编号
		i_header.bucket_size_ = bucket_size;	// 哈希桶大小

		// 索引文件当前偏移(大小)
		i_header.index_file_size_ = sizeof(IndexHeader) + bucket_size * sizeof(int32_t);

		// 实际的情况 indexHead + 总的哈希桶的数据
		char* init_data = new char[i_header.index_file_size_];
		memcpy(init_data, &i_header, sizeof(IndexHeader));
		// 把总的哈希桶的数据清空
		memset(init_data + sizeof(IndexHeader), 0,
			i_header.index_file_size_ - sizeof(IndexHeader));
		// 写索引头和哈希桶到索引文件中
		ret = file_op_->pwrite_file(init_data, i_header.index_file_size_, 0);
		delete[] init_data;
		init_data = NULL;

		if (ret != TFS_SUCCESS)
		{
			return ret;
		}
		ret = file_op_->flush_file();
		if (ret != TFS_SUCCESS)
		{
			return ret;
		}

	}
	else//	索引文件存在
	{
		return EXIT_META_UNEXPECT_FOUND_ERROR;
	}
	// 映射到内存
	ret = file_op_->mmap_file(map_option);
	if (ret != TFS_SUCCESS)
	{
		return ret;
	}
	is_load_ = true;
	return TFS_SUCCESS;
}

int qiniu::largefile::IndexHandle::load(const uint32_t logic_block_id, const int32_t _bucket_size, const MMapOption map_option)
{
	int ret = TFS_SUCCESS;

	if (is_load_)
	{
		return EXIT_INDEX_ALREADY_LOADED_ERROR;
	}
	int file_size = file_op_->get_file_size();
	if (file_size < 0)
	{
		return file_size;
	}
	else if (file_size == 0)
	{
		return EXIT_INDEX_CORRUPT_ERROR;
	}
	MMapOption tmp_map_option = map_option;
	if (file_size > tmp_map_option.first_mmap_size_ && file_size <= tmp_map_option.max_mmap_size_)
	{
		tmp_map_option.first_mmap_size_ = file_size;
	}
	ret = file_op_->mmap_file(tmp_map_option);
	if (ret != TFS_SUCCESS)
	{
		return ret;
	}
	if (debug)
	{
		
		printf("IndexHandle::load - bucket_size():%d, index_header()->bucket_size_: %d, block id: %d\n",
			bucket_size(), index_header()->bucket_size_, block_info()->block_id_);
	}
	if (bucket_size() == 0 || block_info()->block_id_ == 0)
	{
		fprintf(stderr, "Index corrupt error. blockid %u, bucket size %d\n", block_info()->block_id_,bucket_size());
		return EXIT_INDEX_CORRUPT_ERROR;
	}
	// 检查文件大小
	int32_t index_file_size = sizeof(IndexHeader) + bucket_size() * sizeof(int32_t);
	if (index_file_size > file_size)
	{
		fprintf(stderr, "index_file_size > file_size\n");
		return EXIT_INDEX_CORRUPT_ERROR;
	}

	// 检查 块的id
	if (logic_block_id != block_info()->block_id_)
	{
		fprintf(stderr, "block id conflict.\n");
		return EXIT_BLOCK_CONFLICT_ERROR;
	}
	// 检查哈希桶大小
	if (_bucket_size != bucket_size())
	{
		fprintf(stderr, "index configure err. bucket_size != this->bucket_size()\n");
		return EXIT_BUCKET_CONFIGURE_ERROR;
	}
	is_load_ = true;
	if (debug)
	{
		printf("is_load success\n");
	}
	return TFS_SUCCESS;
}

int qiniu::largefile::IndexHandle::remove(const uint32_t logic_block_id)
{
	if (is_load_)
	{
		if (logic_block_id != block_info()->block_id_)
		{
			fprintf(stderr, "block conflict, logic_block_id: %d," 
				"block_info()->block_id_: % d\n", 
				logic_block_id, block_info()->block_id_);
			return EXIT_BLOCK_CONFLICT_ERROR;
		}
	}
	int ret = file_op_->munmap_file();
	if (ret != EXIT_SUCCESS)
	{
		return ret;
	}
	file_op_->unlink_file();
	return ret;
}

int qiniu::largefile::IndexHandle::flush()
{
	int ret = file_op_->flush_file();
	if (ret != EXIT_SUCCESS)
	{
		fprintf(stderr, "index flush fail, ret: %d error desc: %s\n", ret, strerror(errno));
	}
	return ret;
}

int qiniu::largefile::IndexHandle::write_segment_meta(const uint64_t key, MetaInfo& meta)
{
	// 当前位置的偏移量和上一次位置的偏移量
	int32_t current_offset = 0, previous_offset = 0;

	// 思考? key 存在吗? 存在--处理？ 不存在--处理？
	
	//1. 从文件哈希表中查找key是否存在  hash_find(key, current_offset, previous_offset)
	int ret = hash_find(key, current_offset, previous_offset);
	if (ret == TFS_SUCCESS)
	{
		return EXIT_META_UNEXPECT_FOUND_ERROR;
	}
	else if (ret != EXIT_META_NOT_FOUND_ERROR)
	{
		return ret;
	}
	//2. 不存在就写入meta到文件哈希表中 hash_insert(key(哪个哈希桶位置),previous_offset,meta)
	ret = hash_insert(key, previous_offset, meta);
	return ret;
}

int qiniu::largefile::IndexHandle::read_segment_meta(const uint64_t key, MetaInfo& meta)
{
	int32_t current_offset = 0, previous_offset = 0;

	//1. 确定key存放在哈希桶的位置slot
	//int32_t slot = static_cast<int32_t>(key) % bucket_size();

	int32_t ret = hash_find(key, current_offset, previous_offset);
	if (ret == TFS_SUCCESS)
	{
		ret = file_op_->pread_file(reinterpret_cast<char*>(&meta), sizeof(MetaInfo), current_offset);
		return ret;
	}
	else
	{
		return ret;
	}
}

int qiniu::largefile::IndexHandle::hash_find(const uint64_t key, int32_t& current_offset, int32_t& previous_offset)
{
	int ret = TFS_SUCCESS;
	MetaInfo meta_info;
	current_offset = 0, previous_offset = 0;
	//1. 确定key存放在哈希桶的位置slot
	int32_t slot = static_cast<int32_t>(key) % bucket_size();

	//2.读取桶首节点存储的第一个节点的偏移量, 如果偏移量为0, 直接返回 EXIT_META_NOT_FOUND_ERROR

	//3.根据偏移量读取存储的metainfo

	/*4.与key进行比较, 相等则设置current_offset和previous_offset
		并返回TFS_SUCCESS,否则继续执行5
	*/
	/* 5.从metainfo中取得下一个节点的在文件中的偏移量, 如果偏移量为0,
		直接返回 EXIT_META_NOT_FOUND_ERROR, 否则跳转到3循环执行
	*/
	int32_t pos = bucket_slot()[slot];

	while (pos != 0)
	{
		
		ret = file_op_->pread_file(reinterpret_cast<char*>(&meta_info), sizeof(MetaInfo), pos);
		if (ret != TFS_SUCCESS)
		{
			return ret;
		}
		// 进行哈希键值key比较
		if (hash_compare(key, meta_info.get_key()))
		{
			current_offset = pos;
			return TFS_SUCCESS;
		}
		previous_offset = pos;
		pos = meta_info.get_next_meta_offset();
	}
	return EXIT_META_NOT_FOUND_ERROR;
}

int32_t qiniu::largefile::IndexHandle::hash_insert(const uint64_t key, int32_t previous_offset, MetaInfo& meta)
{
	MetaInfo tmp_meta;
	int ret = TFS_SUCCESS;
	//1. 确定key存放在哈希桶的位置slot
	int32_t slot = static_cast<int32_t>(key) % bucket_size();

	//2.确定meta节点存储在文件中的偏移量
	int32_t current_offset = index_header()->index_file_size_;
	// 更新下一个没有存储metainfo的位置
	index_header()->index_file_size_ += sizeof(MetaInfo);

	//3.将meta节点写入到索引文件
	meta.set_next_meta_offset(0);
	ret = file_op_->pwrite_file(reinterpret_cast<const char*>(&meta), sizeof(MetaInfo), current_offset);

	if (ret != TFS_SUCCESS)
	{
		// 回退
		index_header()->index_file_size_ -= sizeof(MetaInfo);
		return ret;
	}
	// 4.将meta节点 插入到哈希链表中
	// 前一个节点已经存在
	if (previous_offset != 0)
	{

		ret = file_op_->pread_file(reinterpret_cast<char*>(&tmp_meta), sizeof(MetaInfo), previous_offset);
		if (ret != TFS_SUCCESS)
		{
			// 回退
			index_header()->index_file_size_ -= sizeof(MetaInfo);
			return ret;
		}
		tmp_meta.set_next_meta_offset(current_offset);
		ret = file_op_->pwrite_file(reinterpret_cast<char*>(&tmp_meta), sizeof(MetaInfo), previous_offset);
		ret = file_op_->pread_file(reinterpret_cast<char*>(&tmp_meta), sizeof(MetaInfo), previous_offset);
		if (ret != TFS_SUCCESS)
		{
			// 回退
			index_header()->index_file_size_ -= sizeof(MetaInfo);
			return ret;
		}
	}
	else
	{
		// current_offset就为第一个偏移
		bucket_slot()[slot] = current_offset;
	}
	return TFS_SUCCESS;
}

int qiniu::largefile::IndexHandle::update_block_info(const OperType oper_type, const uint32_t modify_size)
{
	if (block_info()->block_id_ == 0)
	{
		return EXIT_BLOCKID_ZERO_ERROR;
	}
	if (oper_type == C_OPER_INSERT)
	{
		++block_info()->version_;
		++block_info()->file_count_;
		++block_info()->seq_no_;
		block_info()->size_ += modify_size;
	}
	if (debug)
	{
		printf("OperType:%s\n", oper_type == C_OPER_INSERT? "C_OPER_INSERT" : "C_OPER_DELETE");
	}

	return TFS_SUCCESS;
}






