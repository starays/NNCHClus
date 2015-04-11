//
// Created by zhangray on 15/3/27.
//

#ifndef _NNHCLUS_CLUSTERNODE_H_
#define _NNHCLUS_CLUSTERNODE_H_

#include <string>

namespace cluster {
class ClusterNode {
private:
    int label_;

    int left_child_label_;

    int right_child_label_;

    int basic_node_num_;

    int distance_matrix_label_;

    float distance_;  // distance between left child and right child

    std::string cluster_name_;

public:
    ClusterNode(int label = -1,
            int left_child_label = -1,
            int right_child_label = -1,
            int basic_node_num = 1,
            int distance_matrix_label = -1,
            std::string cluster_name = "")
            : label_(label),
              left_child_label_(left_child_label),
              right_child_label_(right_child_label),
              basic_node_num_(basic_node_num),
              distance_matrix_label_(distance_matrix_label),
              cluster_name_(cluster_name) {}

    void init(int label, int distance_matrix_label, std::string name) {
        label_ = label;
        distance_matrix_label_ = distance_matrix_label;
        cluster_name_ = name;
    }

    int getLabel() const {
        return label_;
    }

    void setLabel(int label) {
        label_ = label;
    }

    int getLeftChildLabel() const {
        return left_child_label_;
    }

    void setLeftChildLabel(int left_child_label) {
        left_child_label_ = left_child_label;
    }

    int getRightChildLabel() const {
        return right_child_label_;
    }

    void setRightChildLabel(int right_child_label) {
        right_child_label_ = right_child_label;
    }

    int getBasicNodeNum() const {
        return basic_node_num_;
    }

    void setBasicNodeNum(int basic_node_num) {
        basic_node_num_ = basic_node_num;
    }

    int getDistanceMatrixLabel() const {
        return distance_matrix_label_;
    }

    void setDistanceMatrixLabel(int distance_matrix_index) {
        distance_matrix_label_ = distance_matrix_index;
    }

    float getDistance() const {
        return distance_;
    }

    void setDistance(float distance) {
        distance_ = distance;
    }

    const std::string &getClusterName() const {
        return cluster_name_;
    }

    void setClusterName(std::string &cluster_name) {
        cluster_name_ = cluster_name;
    }
};
}  // namespace cluster

#endif //_NNHCLUS_CLUSTERNODE_H_
