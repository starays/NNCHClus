//
// Created by zhangray on 15/4/10.
//
#include <cstdio>
#include <cstdlib>

#include "AgglHierClusterer.h"

int main(int argc, char ** argv) {
    if (argc != 5) {
        fprintf(stderr, "Usage: aggl_hiercluser_test distance_file"
                " basic_node_num distance_threshold outfile\n");
        return 1;
    }
    int basic_node_num = atoi(argv[2]);
    int sim_threshold = atof(argv[3]);
    if (basic_node_num <= 0) {
        fprintf(stderr, "Node num must bigger than zero!");
        return 1;
    }
    // for test, use AVERAGE distance function
    cluster::AgglHierClusterer* clusterer = new cluster::AgglHierClusterer();
    fprintf(stderr, "====start to init the clusterer====\n");
    if (!clusterer->init(basic_node_num, argv[1])) {
        fprintf(stderr, "====init clusterer failed!\n");
        return 1;
    }
    fprintf(stderr, "====start to aggregate nodes====\n");
    if (!clusterer->doCluster()) {
        fprintf(stderr, "====cluster nodes failed!\n");
        return 1;
    }
    fprintf(stderr, "====start output to file===\n");
    // put the cluster tree to outfile
    clusterer->output(argv[4]);
    // put the cluster name to outfile
    clusterer->output(std::string(argv[4]) + ".cluster", 0.15);
    delete clusterer;
}
