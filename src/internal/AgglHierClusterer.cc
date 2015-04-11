//
// Created by zhangray on 15/3/27.
//

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include <stack>
#include <queue>
#include "ClusterNode.h"

#include "AgglHierClusterer.h"

namespace cluster {

static const int maxCharBufferSize = 1 << 11;  // input buffer size 8k
static const std::string seg = "\t";

std::vector<std::string> AgglHierClusterer::splitString(
        std::string input,
        std::string seg_pattern) {
    std::string::size_type pos;
    std::vector<std::string> result;
    input += seg_pattern;
    int size = input.size();
    for (int i = 0; i < size; i++) {
        pos = input.find(seg_pattern, i);
        if (pos < size) {
            std::string s = input.substr(i, pos - i);
            result.push_back(s);
            i = pos + seg_pattern.size() - 1;
        }
    }
    return result;
}

/*
* Find the nearest neighbor of node. Return the nearest neighbor's label, while
* distance is a ret-value parameter indicates the distance between node and it's
* nearest neighbor
*/
int AgglHierClusterer::findNearestNeighbor(
        const ClusterNode &node,
        float *distance) {
    int distance_matrix_label = node.getDistanceMatrixLabel();
    assert(distance_matrix_label >= 0);
    float min_distance = -1.0;  // it is not setted
    int min_distance_node_label = -1;
    for (int i = 0; i < node_num_; ++i) {
        if (cluster_node_array_[i].getDistanceMatrixLabel() < 0 ||
                cluster_node_array_[i].getLabel() == node.getLabel()) {
            continue;
        }
        int distance_index =
                getDistanceMatrixIndex(node, cluster_node_array_[i]);
        if (min_distance < 0.0
                || distance_matrix_[distance_index] < min_distance) {
            min_distance = distance_matrix_[distance_index];
            min_distance_node_label = i;
        }
    }
    *distance = min_distance;
    return min_distance_node_label;
}

bool AgglHierClusterer::init(int node_num, const std::string &file_name) {
    assert(node_num > 1);

    base_node_num_ = node_num;
    cluster_node_array_ =
            new(std::nothrow) ClusterNode[(base_node_num_ << 1) -1];
    if (cluster_node_array_ == NULL) {
        return false;
    }

    // then create the matrix of the distance
    size_t distanceEdgeNum =
            (size_t(base_node_num_) * size_t(base_node_num_ - 1)) >> 1;

    distance_matrix_ = new float[distanceEdgeNum];

    if (distance_matrix_ == NULL) {
        return false;
    }

    memset(distance_matrix_, 0, sizeof(float) * distanceEdgeNum);
    // load distance matrix from file
    if (!loadDistanceMatrix(file_name)) {
        fprintf(stderr, "Init of clusterer failed,"
                " cannot load distance matrix correctly\n");
        return false;
    } else {
        fprintf(stderr, "Init success!\n");
    }
    node_num_ = base_node_num_;
    return true;
}

AgglHierClusterer::~AgglHierClusterer() {
    delete distance_calculator_;
    delete[] cluster_node_array_;
    delete[] distance_matrix_;
}

/*
* Load distance matrix from text file. File format like:
*
* ClusterA \t ClusterB \t 0.56
* ClusterC \t ClusterB \t 0.43
* ClusterA \t ClusterC \t 0.32
*
* And there is no necessary of repeated node pairs like <ClusterA, ClusterB>
*     and <ClusterB, ClusterA>
*/
bool AgglHierClusterer::loadDistanceMatrix(const std::string &file_name) {
    size_t expected_pair_num =
            ((size_t)base_node_num_ * (size_t)(base_node_num_ -1)) >> 1;
    assert(cluster_node_array_ != NULL && distance_matrix_ != NULL);
    FILE *distance_file = NULL;
    if ((distance_file = fopen(file_name.c_str(), "r")) == NULL) {
        fprintf(stderr, "Cannot distance file：%s\n", file_name.c_str());
        return false;
    }
    char input_buffer[maxCharBufferSize] = {0};
    int loaded_node_num = 0;
    size_t loaded_pair_num = 0;

    while (fgets(input_buffer, maxCharBufferSize, distance_file) != NULL) {
        std::string line(input_buffer);
        if (line.size() <= 0) {  // empty lines
            continue;
        }
        if (line[0] == '#') {  // comment line
            continue;
        }

        std::vector<std::string> split_result = splitString(line, seg);
        if (split_result.size() != 3) {  // invalid line
            fprintf(stderr, "Invalid line %s", line.c_str());
            continue;
        }

        int left_label = -1;
        int right_label = -1;

        if (node_name_map_.find(split_result[0]) != node_name_map_.end()) {
            left_label = node_name_map_.find(split_result[0])->second;
        } else {
            left_label = loaded_node_num++;
            node_name_map_.insert(
                    std::pair<std::string, int>(split_result[0], left_label));
        }

        if (node_name_map_.find(split_result[1]) != node_name_map_.end()) {
            right_label = node_name_map_.find(split_result[1])->second;
        } else {
            right_label = loaded_node_num++;
            node_name_map_.insert(
                    std::pair<std::string, int>(split_result[1], right_label));
        }

        float distance = atof(split_result[2].c_str());

        if (left_label < 0
                || left_label >= base_node_num_
                || right_label < 0
                || right_label >= base_node_num_) {
            fprintf(stderr, "Invalid label: exceed limit: %d, %d\n",
                    left_label, right_label);
            return false;
        }

        if (left_label == right_label) {
            fprintf(stderr, "Invalid line: %s, the paired nodes is the same!\n", line.c_str());  // NOLINT
            continue;
        }

        if (cluster_node_array_[left_label].getLabel() < 0) {
            cluster_node_array_[left_label].init(
                    left_label, left_label, split_result[0]);
        }

        if (cluster_node_array_[right_label].getLabel() < 0) {
            cluster_node_array_[right_label].init(
                    right_label, right_label, split_result[1]);
        }

        size_t index = getDistanceMatrixIndex(left_label, right_label);

        if (distance_matrix_[index] <= 0.0) {
            distance_matrix_[index] = distance;
            loaded_pair_num++;
            if (loaded_pair_num % 1000000 == 0) {
                fprintf(stderr, "%lu pairs loaded\n", loaded_pair_num);
            }
        }
    }
    if (loaded_node_num != base_node_num_ ||
            loaded_pair_num != expected_pair_num) {
        fprintf(stderr, "Load %d nodes and %lu pairs, load error!\n",
                loaded_node_num, loaded_pair_num);
        return false;
    }
    fprintf(stderr, "Load file success! %d nodes and %lu pairs loaded\n",
            loaded_node_num, loaded_pair_num);
    fclose(distance_file);
    return true;
}

/*
* Aggregate all the node's into one
*/
bool AgglHierClusterer::doCluster() {
    std::stack<int> nearest_neighbor_chain;
    nearest_neighbor_chain.push(0);

    // Current end of the nearest neighbor chain
    ClusterNode* top_node = &cluster_node_array_[nearest_neighbor_chain.top()];

    float nearest_distance = 0.0;
    // The node which ready to add to the nearest neighbor chain
    int nearest_neighbor_label =
            findNearestNeighbor(*top_node, &nearest_distance);
    ClusterNode* nearest_neighbor = &cluster_node_array_[nearest_neighbor_label];

    while (!nearest_neighbor_chain.empty()) {
        if (node_num_ == (base_node_num_ << 1) - 1) {
            fprintf(stderr, "Agglometive hierical cluster success!\n");
            break;
        }
        float next_nearest_distance = 0.0;
        // The next node of the nearest neighbor chain
        int next_node_label = findNearestNeighbor(*nearest_neighbor,
                &next_nearest_distance);

        if (next_node_label == top_node->getLabel()) {
            // aggregate the top node and it's nearest neighbor
            nearest_neighbor_chain.pop();
            int new_node_label = aggregate(top_node,
                    nearest_neighbor,
                    nearest_distance);

            if (nearest_neighbor_chain.empty()) {
                nearest_neighbor_chain.push(new_node_label);
            }
            // then find the nearest neighbor of the top;
            // update the loop's condition
            if (nearest_neighbor_chain.size() == 1) {
                top_node = &cluster_node_array_[nearest_neighbor_chain.top()];
                nearest_neighbor_label = findNearestNeighbor(*top_node,
                                                             &nearest_distance);
            } else {
                // length of current nearest neighbor chain is bigger than 2
                nearest_neighbor_label = nearest_neighbor_chain.top();
                nearest_neighbor_chain.pop();
                top_node = &cluster_node_array_[nearest_neighbor_chain.top()];
                nearest_distance = distance_matrix_[getDistanceMatrixIndex(
                        top_node->getDistanceMatrixLabel(),
                        nearest_neighbor_label
                )];
            }
        } else {  // push the next node in to stack and goto next
            nearest_neighbor_chain.push(nearest_neighbor_label);
            top_node = nearest_neighbor;
            nearest_neighbor_label = next_node_label;
            nearest_distance = next_nearest_distance;
        }
        nearest_neighbor = &cluster_node_array_[nearest_neighbor_label];
    }
    return true;
}

/*
* Aggregate two cluster nodes to a new node.
* The new node will be pushed into the cluster node array.
* And then the distance matrix will be updated
*/
int AgglHierClusterer::aggregate(
        ClusterNode *left_node,
        ClusterNode *right_node,
        float distance) {
    assert(left_node != NULL && right_node != NULL);
    // new node
    int left_node_dis_label = left_node->getDistanceMatrixLabel();
    int right_node_dis_label = right_node->getDistanceMatrixLabel();
    left_node->setDistanceMatrixLabel(-1);
    right_node->setDistanceMatrixLabel(-1);
    int new_node_dis_label =
            left_node_dis_label < right_node_dis_label ?
                    left_node_dis_label : right_node_dis_label;
    cluster_node_array_[node_num_].setLabel(node_num_);
    cluster_node_array_[node_num_].setBasicNodeNum(
            left_node->getBasicNodeNum() + right_node->getBasicNodeNum());
    cluster_node_array_[node_num_].setLeftChildLabel(left_node->getLabel());
    cluster_node_array_[node_num_].setRightChildLabel(right_node->getLabel());  // NOLINT
    cluster_node_array_[node_num_].setDistanceMatrixLabel(new_node_dis_label);  // NOLINT
    cluster_node_array_[node_num_].setDistance(distance);

    // then update the distance matrix
    int cur_dis_label = -1;
    float cur_left_dis = 0.0;
    float cur_right_dis = 0.0;
    float cur_new_dis = 0.0;
    for (int i = 0; i < node_num_; ++ i) {
        cur_dis_label = cluster_node_array_[i].getDistanceMatrixLabel();
        if (cur_dis_label < 0) {  // current node has been aggregated;
            continue;
        }
        cur_left_dis = distance_matrix_[getDistanceMatrixIndex(
                left_node_dis_label, cur_dis_label)];
        cur_right_dis = distance_matrix_[getDistanceMatrixIndex(
                right_node_dis_label, cur_dis_label)];
        cur_new_dis = (*distance_calculator_)(left_node->getBasicNodeNum(),
                right_node->getBasicNodeNum(),
                cluster_node_array_[i].getBasicNodeNum(),
                cur_left_dis,
                cur_right_dis,
                distance
        );
        distance_matrix_[getDistanceMatrixIndex(cur_dis_label, new_node_dis_label)] = cur_new_dis;  // NOLINT
    }

    // update distance matrix end; merge complete
    node_num_ ++;
    return cluster_node_array_[node_num_ - 1].getLabel();
}

/*
* Output the whole agglomerative tree to a file.
*/
bool AgglHierClusterer::output(const std::string &file_name) {
    FILE * out_file = NULL;
    if ((out_file = fopen(file_name.c_str(), "w")) == NULL) {
        fprintf(stderr, "Open file: %s failed!", file_name.c_str());
        return false;
    }
    fprintf(out_file,
            "ClusterLabel\tClusterName\tLeftChildLabel\tRightChildLabel\tDistance\n"); // NOLINT
    // just output the nodes and its child from end of the cluster node array to
    // begin
    for (int i = node_num_ - 1; i >=0; -- i) {
        ClusterNode & cur_node = cluster_node_array_[i];
        std::string cluster_name = cur_node.getClusterName();
        if (cur_node.getLeftChildLabel() >= 0
                && cur_node.getRightChildLabel() >= 0) {
            cluster_name = "NOT_LEAF_NODE";
        }
        fprintf(out_file,
                "%d\t%s\t%d\t%d\t%f\n",
                cur_node.getLabel(),
                cluster_name.c_str(),
                cur_node.getLeftChildLabel(),
                cur_node.getRightChildLabel(),
                cur_node.getDistance()
        );
    }

    fclose(out_file);
    return true;
}

/*
* Output the cluster to a file.
*/
bool AgglHierClusterer::output(const std::string &file_name,
        float distance_threshold) {
    FILE * out_file = NULL;
    if ((out_file = fopen(file_name.c_str(), "w")) == NULL) {
        fprintf(stderr, "Open file: %s failed!\n", file_name.c_str());
        return false;
    }
    fprintf(out_file, "ClusterSize\tClusterNodes[1,2,3...]\n");
    std::queue<int> cluster_nodes_queue;
    cluster_nodes_queue.push(node_num_ - 1);
    int cur_label = -1;
    while (! cluster_nodes_queue.empty()) {
        cur_label = cluster_nodes_queue.front();
        cluster_nodes_queue.pop();

        assert(cur_label > 0 && cur_label < node_num_);

        ClusterNode& cur_cluster_node = cluster_node_array_[cur_label];
        if (cur_cluster_node.getLeftChildLabel() < 0 &&
                cur_cluster_node.getRightChildLabel() < 0) {
            // leaf nodes
            fprintf(out_file,
                    "1\t%s\n",
                    cur_cluster_node.getClusterName().c_str());
        } else if (cur_cluster_node.getDistance() > distance_threshold) {
            cluster_nodes_queue.push(cur_cluster_node.getLeftChildLabel());
            cluster_nodes_queue.push(cur_cluster_node.getRightChildLabel());
        } else {
            // output all the children of the nodes
            std::queue<int> out_queue;
            std::vector<int> out_labels;
            out_queue.push(cur_label);
            int cur_out_label = -1;
            while (!out_queue.empty()) {
                cur_out_label = out_queue.front();
                out_queue.pop();
                ClusterNode & cur_out_node = cluster_node_array_[cur_out_label];
                if (cur_out_node.getLeftChildLabel() < 0 &&
                        cur_out_node.getRightChildLabel() < 0) {
                    out_labels.push_back(cur_out_label);
                } else {
                    // not a leaf node
                    out_queue.push(cur_out_node.getLeftChildLabel());
                    out_queue.push(cur_out_node.getRightChildLabel());
                }
            }
            fprintf(out_file, "%lu", out_labels.size());
            for (std::vector<int>::const_iterator it = out_labels.begin();
                    it != out_labels.end(); ++ it) {
                fprintf(out_file,
                        "\t%s",
                        cluster_node_array_[(*it)].getClusterName().c_str());
            }
            fprintf(out_file, "\n");
        }
    }
    fclose(out_file);
    return true;
}

}  // namespace cluster

