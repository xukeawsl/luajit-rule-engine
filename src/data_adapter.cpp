#include "ljre/data_adapter.h"


namespace ljre {

// 静态成员初始化
std::atomic<uint64_t> DataAdapter::_next_id{1};

} // namespace ljre
