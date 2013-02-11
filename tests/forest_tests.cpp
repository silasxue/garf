#include "gtest/gtest.h"
// #include <glog/logging.h>


#include <iostream>
#include <fstream>

#include <Eigen/Dense>
#include <Eigen/Core>

using Eigen::Matrix;
using Eigen::RowVectorXd;
using Eigen::Vector3d;
using Eigen::VectorXd;
using Eigen::Matrix3d;
using Eigen::MatrixXd;

#define GARF_SERIALIZE_ENABLE

#include "garf/regression_forest.hpp"


typedef garf::RegressionForest<garf::AxisAlignedSplt, garf::AxisAlignedSplFitter> forest_ax_align;

const double tol = 0.00001;

void make_1d_labels_from_2d_data_squared_diff(const MatrixXd & features, MatrixXd & labels) {
    labels.col(0) = features.col(0).cwiseProduct(features.col(0)) - 
                    0.5 * features.col(1).cwiseProduct(features.col(1));
}

void make_1d_labels_from_1d_data_abs(const MatrixXd & features, MatrixXd & labels) {
    labels.col(0) = features.col(0).cwiseAbs();
}

template<typename T, int num_rows, int num_cols>
void expect_matrices_equal(const Matrix<T, num_rows, num_cols> & m1,
                           const Matrix<T, num_rows, num_cols> & m2) {
    garf::eigen_idx_t rows = m1.rows();
    garf::eigen_idx_t cols = m1.cols();
    // Making these an assert because these really should never happen!
    if (rows != m2.rows()) {
        ASSERT_FALSE("matrices do not match in size!");
    } else if (cols != m2.cols()) {
        ASSERT_FALSE("matrices do not match in size!");
    }
    for (garf::eigen_idx_t i = 0; i < rows; i++){
        for (garf::eigen_idx_t j = 0; j < cols; j++) {
            EXPECT_EQ(m1(i, j), m2(i, j));
        }
    }
}


template<class ForestT>
void assert_forest_predictions_match(const ForestT & f1,
                                     const ForestT & f2,
                                     const garf::feature_matrix & features) {
    garf::eigen_idx_t num_datapoints = features.rows();

    // Create output arrays to store everything in
    const garf::ForestStats & stats = f1.stats();
    garf::label_matrix l1(num_datapoints, stats.label_dimensions);
    garf::label_matrix l2(num_datapoints, stats.label_dimensions);
    garf::variance_matrix v1(num_datapoints, stats.label_dimensions);
    garf::variance_matrix v2(num_datapoints, stats.label_dimensions);
    garf::tree_idx_matrix t1(num_datapoints, stats.num_trees);
    garf::tree_idx_matrix t2(num_datapoints, stats.num_trees);

    // return;

    f1.predict(features, &l1, &v1, &t1);
    

    f2.predict(features, &l2, &v2, &t2);

    // If the forests are the same, they should produce the same
    // means, variances and output indices
    expect_matrices_equal(l1, l2);
    expect_matrices_equal(v1, v2);
    expect_matrices_equal(t1, t2);

    std::cout << "forest outputs are equal!" << std::endl;
}



// Given a forest with training parameters already set, clear the forest,
// generate a bunch of random training data, generate labels from this
// data with the provided function pointer and other params, then check that the forest
// 
void test_forest_with_data(forest_ax_align & forest,
                           void (* label_generator)(const MatrixXd &, MatrixXd &),
                           uint64_t num_train_datapoints, uint64_t num_test_datapoints,
                           uint64_t data_dims, uint64_t label_dims,
                           double data_scaler, double noise_variance,
                           double answer_tolerance) {
    forest.clear();

    // Generate the (uniformly distributed data)
    MatrixXd train_data(num_train_datapoints, data_dims);
    train_data.setRandom();
    train_data *= data_scaler;

    // Generate labels, add noise
    MatrixXd train_labels(num_train_datapoints, label_dims);
    label_generator(train_data, train_labels);
    MatrixXd noise(num_train_datapoints, label_dims);
    noise.setRandom();
    train_labels += noise_variance * noise;

    // Train forest
    forest.clear();
    forest.train(train_data, train_labels);

    // Make test data
    MatrixXd test_data(num_test_datapoints, data_dims);
    test_data.setRandom();
    test_data *= data_scaler;

    // Test labels
    MatrixXd test_labels(num_test_datapoints, label_dims);
    label_generator(test_data, test_labels);

    MatrixXd predicted_labels(num_test_datapoints, label_dims);
    forest.predict(test_data, &predicted_labels);

    for (uint64_t i = 0; i < num_test_datapoints; i++) {
        for (uint64_t j = 0; j < label_dims; j++) {
            EXPECT_NEAR(predicted_labels(i, j), test_labels(i, j), answer_tolerance);
        }
    }
}

TEST(ForestTest, RegTest1) {
    forest_ax_align forest;
    forest.forest_options.max_num_trees = 10;
    forest.tree_options.max_depth = 6;
    forest.tree_options.min_sample_count = 2;

    uint64_t num_train_datapoints = 1000;
    uint64_t num_test_datapoints = 100;
    uint64_t data_dims = 2;
    uint64_t label_dims = 1;
    double data_scaler = 2.0;
    double noise_variance = 0.1;
    double answer_tolerance= 1.0;

    // 1D data, 1D labels
    test_forest_with_data(forest, make_1d_labels_from_2d_data_squared_diff,
                          num_train_datapoints, num_test_datapoints,
                          data_dims, label_dims, data_scaler,
                          noise_variance, answer_tolerance);
}

TEST(ForestTest, RegTest2) {
    forest_ax_align forest;
    forest.forest_options.max_num_trees = 10;
    forest.tree_options.max_depth = 6;
    forest.tree_options.min_sample_count = 2;

    uint64_t num_train_datapoints = 1000;
    uint64_t num_test_datapoints = 100;
    uint64_t data_dims = 1;
    uint64_t label_dims = 1;
    double data_scaler = 2.0;
    double noise_variance = 0.1;
    double answer_tolerance = 0.1;

    // 1D data, 1D labels
    test_forest_with_data(forest, make_1d_labels_from_1d_data_abs,
                          num_train_datapoints, num_test_datapoints,
                          data_dims, label_dims, data_scaler,
                          noise_variance, answer_tolerance);
}

TEST(ForestTest, Serialize) {
    uint64_t data_elements = 1000;
    uint64_t data_dims = 2;
    uint64_t label_dims = 1;
    MatrixXd data(data_elements, data_dims);
    data.setRandom();

    MatrixXd labels(data_elements, label_dims);
    make_1d_labels_from_2d_data_squared_diff(data, labels);
    labels.setRandom();

    forest_ax_align forest1;
    forest1.forest_options.max_num_trees = 10;
    forest1.tree_options.max_depth = 6;
    forest1.tree_options.min_sample_count = 2;
    forest_ax_align forest2;


    forest1.train(data, labels);
    forest1.save_forest("test_serialize.forest");
    forest2.load_forest("test_serialize.forest");

    assert_forest_predictions_match<forest_ax_align>(forest1, forest2, data);
}

GTEST_API_ int main(int argc, char **argv) {
    // Print everything, including INFO and WARNING
    // FLAGS_stderrthreshold = 0;
    // google::InitGoogleLogging(argv[0]);
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}