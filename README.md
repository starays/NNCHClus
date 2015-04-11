#NNCHClus

**NNCHClus(Nearest Neighbor Chain Based Hierarchical Cluster)** 是一个快速的聚合层次聚类算法。在本项目的 `reference` 中，给出了该算法的一个详细描述。如果不愿意读 `reference` 中的论文的话，你也可以在 [Wikipedia](http://en.wikipedia.org/wiki/Nearest-neighbor_chain_algorithm) 上找到该算法原理的一个简单描述。

传统的聚合层次聚类采用**贪心算法**，算法的大致过程是：1. **初始化**：把每个样本归为一类，计算每两个类之间的距离，也就是样本与样本之间的相似度；2. 寻找各个类之间最近的两个类，把他们归为一类（这样类的总数就少了一个）；3. 重新计算新生成的这个类与各个旧类之间的相似度；4. 重复2和3直到所有样本点都归为一类，结束。
这种算法的时间复杂度为 `O(n^3)` 或者 `O(n^2*log(n))`，在较大的数据上，算法运行的时间较长。NNCHClus 使用了 `Nearest Neighbor Chain` 这样一种数据结构来对算法进行加速，具体加速的算法可以参看本项目的 `reference`。NNCHClus 要求类簇之间的距离满足 `可归约性(reducibility)`，即：
>对于 A，B，C 三个类簇，A 和 B 合并后新的类簇与 C 之间的距离应该满足：```dis(A ∪ B, C) ≥ min(dis(A,C), dis(B,C))
```
对于绝大部分的距离函数，如凝聚层次聚类中常见的 **Single-Linkage**， **Complete-Linkage**，**Average-Linkage**，**质心法**，**Ward**等距离函数，都能满足以上可规约性的约束条件。


##Usage
NNCHClus 算法被封装在 `AgglHierClusterer` 中。可以通过指定距离函数的来创建AgglHierClusterer：

```
cluster::AgglHierClusterer * clusterer =
    new cluster::AgglHierClusterer(clusterer::DistanceCalculatorType::CENTROID);
```

目前实现的距离函数包括：

```
class DistanceCalculatorType{
public:
    enum Type {
        SINGLE_LINK = 0,
        COMPLETE_LINK,
        CENTROID,
        AVERAGE,
        WARD
    };
};
```

然后指定 sample 的数量和距离文件的地址，对 AgglHierClusterer 初始化：

```
if (!clusterer->init(basic_node_num, "distance_matrix_file")) {
    fprintf(stderr, "====init clusterer failed!\n");
    return 1;
}
```

**distance_matrix_file** 存储了 samples 两两之间的距离。格式如下：

>ClusterA \t ClusterB \t 0.56

>ClusterC \t ClusterB \t 0.43

>ClusterA \t ClusterC \t 0.32

对于大数据集，求距离矩阵也是一个非常耗时的操作。好在可以通过 **Hadoop** 等并行计算方法来加速这一过程。

init 成功之后可以通过 `doCluster()` 来进行聚类。得到的结果通过 `output()` 函数输出。

```
    fprintf(stderr, "====start output to file===\n");
    // put the cluster tree to outfile
    clusterer->output(argv[4]);
    // put the cluster name to outfile
    clusterer->output(std::string(argv[4]) + ".cluster", 0.15);
```

```output(std::string file)``` 输出了一颗完整的层次树。输出的每一行包括书上的一个节点，及其左右子数的编号，左右子树合并的时候两者之间的距离，等。

```output(std::string file, float distance_threshod)``` 则将距离小于 distance_threshold 的节点当作一个类簇输出。
##性能分析
NNCHClus 的空间复杂度是`O(n^2)`,确切地说，需要加载一个 `n * (n-1) / 2 * sizeof(float) ` 的距离矩阵。对于 10w 个 sample，距离矩阵占用内存大概为 20G。
性能测试的 Benchmark 还没有进行过，对于 1w 个 sample，聚类所花的时间为 20s 左右。

##可能的改进方案
+ 当前的性能瓶颈主要集中在加载距离矩阵上。加载距离矩阵的时间比聚类花的时间多了不少。可以通过加载二进制文件的方式提高加载效率。
+ output 的方式比较不友好。可以尝试使用 [DendroGram](https://github.com/ChrisBeaumont/DendroDocs/blob/master/cpp.rst) 或者 [astrodendro](http://www.dendrograms.org/en/latest/) 来生成易于理解的图片。