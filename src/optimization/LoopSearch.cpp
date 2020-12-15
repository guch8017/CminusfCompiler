#include "LoopSearch.hpp"
#include <iostream>
#include <unordered_set>
#include <fstream>
#include "logging.hpp"

struct CFGNode
{
    std::unordered_set<CFGNodePtr> succs;
    std::unordered_set<CFGNodePtr> prevs;
    BasicBlock *bb;
    int index;   // the index of the node in CFG
    int lowlink; // the min index of the node in the strongly connected componets
    bool onStack;
};

// build control flow graph used in loop search pass
void LoopSearch::build_cfg(Function *func, std::unordered_set<CFGNode *> &result)
{
    std::unordered_map<BasicBlock *, CFGNode *> bb2cfg_node;  // Block->CFG节点映射表(Map)，用于查找CFGNode
    for (auto bb : func->get_basic_blocks())
    {
        // 遍历所有基本块，每个基本块作为一个CFG顶点，构造CFG
        // CFG存储方式为邻接链表形式
        auto node = new CFGNode;
        node->bb = bb;
        node->index = node->lowlink = -1;
        node->onStack = false;
        bb2cfg_node.insert({bb, node});

        result.insert(node);
    }
    for (auto bb : func->get_basic_blocks())
    {
        auto node = bb2cfg_node[bb];  // 获取Block对应的CFGNode
        std::string succ_string = "success node: ";
        for (auto succ : bb->get_succ_basic_blocks())
        {
            succ_string = succ_string + succ->get_name() + " ";
            node->succs.insert(bb2cfg_node[succ]);  // 后继节点
        }
        std::string prev_string = "previous node: ";
        for (auto prev : bb->get_pre_basic_blocks())
        {
            prev_string = prev_string + prev->get_name() + " ";
            node->prevs.insert(bb2cfg_node[prev]);  // 前继节点
        }
    }
}

// Tarjan algorithm
// reference: https://baike.baidu.com/item/tarjan%E7%AE%97%E6%B3%95/10687825?fr=aladdin
/**
 * 强连通量搜索算法实现
 * 
 * 注意: 
 * 本函数实现的算法给出的强连通分量与定义略有不同。单个节点在此处不被认为是强连通分量。
 * 
 * @param nodes CFG的邻接链表表示
 * @param result 搜索到的所有强连通分量
 * @return nodes构成的CFG中是否包含强连通分量
 */
bool LoopSearch::strongly_connected_components(
    CFGNodePtrSet &nodes,
    std::unordered_set<CFGNodePtrSet *> &result)
{
    index_count = 0;
    stack.clear();
    for (auto n : nodes)
    {
        if (n->index == -1)
            traverse(n, result);
    }
    return result.size() != 0;
}
void LoopSearch::traverse(
    CFGNodePtr n,
    std::unordered_set<CFGNodePtrSet *> &result)
{
    n->index = index_count++;   // 深搜次序
    n->lowlink = n->index;      // LOW 标记
    stack.push_back(n);         // DFS栈
    n->onStack = true;          // 标记在栈中

    for (auto su : n->succs)
    {
        // has not visited su
        if (su->index == -1)
        {
            traverse(su, result);
            n->lowlink = std::min(su->lowlink, n->lowlink);
        }
        // has visited su
        else if (su->onStack)
        {
            n->lowlink = std::min(su->index, n->lowlink);
        }
        // 已搜索并添加至result的节点上述两种情况都不符合
    }

    // nodes that in the same strongly connected component will be popped out of stack
    if (n->index == n->lowlink)
    {
        // 找到强连通分量
        auto set = new CFGNodePtrSet;
        CFGNodePtr tmp;
        // 弹出n的强连通分量包含的所有节点
        do
        {
            tmp = stack.back();
            tmp->onStack = false;
            set->insert(tmp);
            stack.pop_back();
        } while (tmp != n);
        // 本实现不将单个节点视为强连通分量
        if (set->size() == 1)
            delete set;
        else
            result.insert(set);
    }
}
CFGNodePtr LoopSearch::find_loop_base(
    CFGNodePtrSet *set,
    CFGNodePtrSet &reserved)
{

    CFGNodePtr base = nullptr;
    bool hadBeen = false;
    for (auto n : *set)
    {
        for (auto prev : n->prevs)
        {
            // 没找到（BasicBlock来自强连通分量外部）
            if (set->find(prev) == set->end())
            {
                base = n;
            }
        }
    }
    if (base != nullptr)
        return base;
    // 内层循环嵌套情况
    for (auto res : reserved)
    {
        for (auto succ : res->succs)
        {
            if (set->find(succ) != set->end())
            {
                base = succ;
            }
        }
    }

    return base;
}

// 优化方法入口
void LoopSearch::run()
{
    // 获取所有函数声明
    auto func_list = m_->get_functions();
    for (auto func : func_list)
    {
        
        if (func->get_basic_blocks().size() == 0)
        {
            // 跳过空函数
            continue;
        }
        else
        {
            CFGNodePtrSet nodes;
            CFGNodePtrSet reserved;
            std::unordered_set<CFGNodePtrSet *> sccs;
            
            // step 1: build cfg
            // 顶点存储位置为nodes，边存储在顶点结构体内
            build_cfg(func, nodes);
            // dump graph（可选）
            dump_graph(nodes, func->get_name());
            // step 2: find strongly connected graph from external to internal
            int scc_index = 0;
            while (strongly_connected_components(nodes, sccs))
            {
                // 函数CFG中包含强连通分量
                if (sccs.size() == 0)
                {
                    break;  // 貌似没用。为0时无法进入此循环
                }
                else
                {
                    // step 3: find loop base node for each strongly connected graph
                    for (auto scc : sccs)
                    {
                        // 遍历所有强连通分量
                        scc_index += 1;
                        // 获取循环入口
                        auto base = find_loop_base(scc, reserved);

                        // step 4: store result
                        auto bb_set = new BBset_t;
                        std::string node_set_string = "";
                        // 遍历强连通分量的所有BasicBlock并插入bb_set
                        for (auto n : *scc)
                        {
                            bb_set->insert(n->bb);
                            node_set_string = node_set_string + n->bb->get_name() + ',';
                        }

                        loop_set.insert(bb_set);                // 所有循环的BasicBlock集合
                        func2loop[func].insert(bb_set);         // 该函数体包含的所有循环的BasicBlock集合
                        base2loop.insert({base->bb, bb_set});   // 由循环入口Block获取Set
                        loop2base.insert({bb_set, base->bb});   // 由Set获取循环入口
                        for (auto bb : *bb_set)                 // 由循环体中任意块获取Set
                        {
                            if (bb2base.find(bb) == bb2base.end())
                            {
                                bb2base.insert({bb, base->bb});
                            }
                            else
                            {
                                bb2base[bb] = base->bb;
                            }
                        }
                        // step 5: map each node to loop base
                        for (auto bb : *bb_set)                 // 由循环体中任意块获取entry
                        {
                            if (bb2base.find(bb) == bb2base.end())
                                bb2base.insert({bb, base->bb});
                            else
                                bb2base[bb] = base->bb;
                        }

                        // 处理循环嵌套
                        // step 6: remove loop base node for researching inner loop
                        reserved.insert(base);                  
                        dump_graph(*scc, func->get_name() + '_' + std::to_string(scc_index));
                        nodes.erase(base);
                        for (auto su : base->succs)
                        {
                            su->prevs.erase(base);
                        }
                        for (auto prev : base->prevs)
                        {
                            prev->succs.erase(base);
                        }

                    } // for (auto scc : sccs)
                    for (auto scc : sccs)
                        delete scc;
                    sccs.clear();
                    // 初始化node属性，准备下一轮判断
                    for (auto n : nodes)
                    {
                        n->index = n->lowlink = -1;
                        n->onStack = false;
                    }
                } // else
            }     // while (strongly_connected_components(nodes, sccs))
            // clear
            reserved.clear();
            for (auto node : nodes)
            {
                delete node;
            }
            nodes.clear();
        } // else

    } // for (auto func : func_list)
}

void LoopSearch::dump_graph(CFGNodePtrSet &nodes, std::string title)
{
    if (dump)
    {
        std::vector<std::string> edge_set;
        for (auto node : nodes)
        {
            if (node->bb->get_name() == "")
            {
                
                return;
            }
            if (base2loop.find(node->bb) != base2loop.end())
            {

                for (auto succ : node->succs)
                {
                    if (nodes.find(succ) != nodes.end())
                    {
                        edge_set.insert(edge_set.begin(), '\t' + node->bb->get_name() + "->" + succ->bb->get_name() + ';' + '\n');
                    }
                }
                edge_set.insert(edge_set.begin(), '\t' + node->bb->get_name() + " [color=red]" + ';' + '\n');
            }
            else
            {
                for (auto succ : node->succs)
                {
                    if (nodes.find(succ) != nodes.end())
                    {
                        edge_set.push_back('\t' + node->bb->get_name() + "->" + succ->bb->get_name() + ';' + '\n');
                    }
                }
            }
        }
        std::string digragh = "digraph G {\n";
        for (auto edge : edge_set)
        {
            digragh += edge;
        }
        digragh += '}';
        std::ofstream file_output;
        file_output.open(title + ".dot", std::ios::out);
        
        file_output << digragh;
        file_output.close();
        std::string dot_cmd = "dot -Tpng " + title + ".dot" + " -o " + title + ".png";
        std::system(dot_cmd.c_str());
    }
}

BBset_t *LoopSearch::get_parent_loop(BBset_t *loop)
{
    auto base = loop2base[loop];
    for (auto prev : base->get_pre_basic_blocks())
    {
        if (loop->find(prev) != loop->end())
            continue;
        auto loop = get_inner_loop(prev);
        if (loop == nullptr || loop->find(base) == loop->end())
            return nullptr;
        else
        {
            return loop;
        }
    }
    return nullptr;
}

std::unordered_set<BBset_t *> LoopSearch::get_loops_in_func(Function *f)
{
    return func2loop.count(f) ? func2loop[f] : std::unordered_set<BBset_t *>();
}
