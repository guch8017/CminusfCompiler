#include <algorithm>
#include "logging.hpp"
#include "LoopSearch.hpp"
#include "LoopInvHoist.hpp"

void LoopInvHoist::run()
{
    // 先通过LoopSearch获取循环的相关信息
    LoopSearch loop_searcher(m_, false);
    loop_searcher.run();

    // 接下来由你来补充啦！
}