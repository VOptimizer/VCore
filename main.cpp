#include <iostream>
#include <VoxelOptimizer/VoxelOptimizer.hpp>
#include <VoxelOptimizer/Loaders/Octree.hpp>

using namespace std;
using namespace VoxelOptimizer;

int main(int argc, char const *argv[])
{
    VoxelOptimizer::COctree<int> tree(CVector(8, 8, 8));
    tree.insert({CVector(3, 3, 3), 5});
    tree.insert({CVector(6, 6, 3), 7});
    tree.insert({CVector(2, 2, 3), 8});
    tree.insert({CVector(1, 1, 1), 1});

    auto it = tree.find(CVector(3, 3, 3));
    if(it != tree.end())
    {
        int i = it->second;
        i++;
    }

    it = tree.find(CVector(3, 3, 4));
    if(it != tree.end())
    {
        int i = 0;
        i++;
    }

    // it = tree.begin();

    for (auto &&i : tree)
    {
        cout << i.second << endl;
    }
    

    return 0;
}
