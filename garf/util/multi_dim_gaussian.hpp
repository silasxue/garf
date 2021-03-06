#ifndef GARF_UTIL_MULTI_DIM_GAUSSIAN_HPP
#define GARF_UTIL_MULTI_DIM_GAUSSIAN_HPP

// #include <iostream>
#include <stdexcept>

#include "../types.hpp"

#ifdef GARF_PYTHON_BINDINGS_ENABLE
#include "../util/python_eigen.hpp"
#endif

namespace garf { namespace util {

    // if we don't know the dimensionality in advance
    template<typename T>
    class MultiDimGaussianX {
        template <typename T1> using mtx = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
        template <typename T1> using vec = Eigen::Matrix<T, Eigen::Dynamic, 1>;
    public:
        const eigen_idx_t dimensions;
        vec<T> mean;
        mtx<T> cov;

        inline MultiDimGaussianX() : dimensions(1) {
            initialise_params();
        }

        inline MultiDimGaussianX(eigen_idx_t _dimensions) : dimensions(_dimensions) {
            initialise_params();
        }

        inline void initialise_params() {
            mean = vec<T>(dimensions, 1);
            cov = mtx<T>(dimensions, dimensions);
            mean.setZero();
            cov.setZero();
        }

        inline MultiDimGaussianX(eigen_idx_t _dimensions,
                                 vec<T> _mean,
                                 mtx<T> _cov) : dimensions(_dimensions) {
            // Check size of the provided mean and cov..
            // This complicated test allows to have the mean vector either way round
            if (!(((_mean.rows() == dimensions) && (_mean.cols() == 1)) ||
                  ((_mean.rows() == 1) && (_mean.cols() == dimensions)))) {
                throw std::invalid_argument("provided mean is wrong shape");
            }
            if ((_cov.rows() != dimensions) || (_cov.cols() != dimensions)) {
                throw std::invalid_argument("provided cov is wrong shape");
            }

            // Store the mean as a column vector            
            mean = _mean;
            mean.resize(dimensions, 1);
            cov = _cov;
        }

        inline void check_data_dimensionality(const mtx <T> & input_data) const {
            eigen_idx_t input_data_dimensionality = input_data.cols();
            if (input_data_dimensionality != dimensions) {
                throw std::invalid_argument("input data dimensionality doesn't match in fit_params");   
            }
        }

        inline void fit_params(const mtx<T> & input_data) {
            check_data_dimensionality(input_data);
            eigen_idx_t num_input_datapoints = input_data.rows();

            // temporary for the efficient / stable computation:
            // http://www.johndcook.com/standard_deviation.html 
            vec<T> mean_k_minus_1(dimensions, 1);
            initialise_params();

            mean = input_data.row(0);

            // recurrence relation, see http://prod.sandia.gov/techlib/access-control.cgi/2008/086212.pdf
            for (eigen_idx_t k = 1; k < num_input_datapoints; k++) {
                mean_k_minus_1 = mean;
                // cov_k_minus_1 = cov;

                // update the mean
                mean = mean_k_minus_1 + (input_data.row(k).transpose() - mean_k_minus_1) / static_cast<double>(k+1);

                // update the covariance
                for (eigen_idx_t d1 = 0; d1 < dimensions; d1++) {
                    double d1_diff = input_data(k, d1) - mean_k_minus_1(d1);
                    for(eigen_idx_t d2 = d1; d2 < dimensions; d2++) {
                        double addition = (k / static_cast<double>(k+1)) * d1_diff
                                        * (input_data(k, d2) - mean_k_minus_1(d2));
                        cov(d1, d2) += addition;
                        cov(d2, d1) = cov(d1, d2); // could make this more efficient by only doing it at the end
                    }
                }

                // We have only calculated the upper triangular portion of the answer. Copy into the lower triangular bit
                for (eigen_idx_t d1 = 0; d1 < dimensions; d1++) { 
                    for (eigen_idx_t d2 = (d1 + 1); d2 < dimensions; d2++) {
                        cov(d2, d1) = cov(d1, d2);
                    }
                }
            }

            if (num_input_datapoints > 1) {
                cov /= (num_input_datapoints - 1);
            }
        }

        // As above, but allows us to also pass a vector of indices indicating only
        //   certain rows of the data matrix should be considered 
        inline void fit_params(const mtx<T> & input_data, const data_indices_vec & valid_indices) {
            check_data_dimensionality(input_data);
            eigen_idx_t num_input_datapoints = valid_indices.size();

            vec<T> mean_k_minus_1(dimensions, 1);
            initialise_params();

            mean = input_data.row(valid_indices(0));

            for (eigen_idx_t k = 1; k < num_input_datapoints; k++) {
                int data_idx = valid_indices(k);
                mean_k_minus_1 = mean;

                // update the mean
                mean = mean_k_minus_1 + (input_data.row(data_idx).transpose() - mean_k_minus_1) / static_cast<double>(k + 1);

                // update the covariance
                for (eigen_idx_t d1 = 0; d1 < dimensions; d1++) {
                    double d1_diff = input_data(data_idx, d1) - mean_k_minus_1(d1);
                    for(eigen_idx_t d2 = d1; d2 < dimensions; d2++) {
                        double addition = (k / static_cast<double>(k+1)) * d1_diff
                                        * (input_data(data_idx, d2) - mean_k_minus_1(d2));
                        cov(d1, d2) += addition;
                        cov(d2, d1) = cov(d1, d2); // could make this more efficient by only doing it at the end
                    }
                }

                // We have only calcualted the upper triangular portion of the answer. Copy into the lower triangular bit
                for (eigen_idx_t d1 = 0; d1 < dimensions; d1++) { 
                    for (eigen_idx_t d2 = (d1 + 1); d2 < dimensions; d2++) {
                        cov(d2, d1) = cov(d1, d2);
                    }
                }
            }
        }

        // As above again, but if only some of the indices in valid_indices are valid. For memory efficiency,
        // sometimes it is better to allocate a vector that is too big, and then only use the first few elements)
        inline void fit_params(const mtx<T> & input_data, const data_indices_vec & valid_indices, const eigen_idx_t num_input_datapoints) {
            check_data_dimensionality(input_data);

            if (num_input_datapoints == 0) {
                throw std::invalid_argument("num_input_datapoints cannot be zero");
            }

            vec<T> mean_k_minus_1(dimensions, 1);
            initialise_params();

            mean = input_data.row(valid_indices(0));

            for (eigen_idx_t k = 1; k < num_input_datapoints; k++) {
                int data_idx = valid_indices(k);
                mean_k_minus_1 = mean;

                // update the mean
                mean = mean_k_minus_1 + (input_data.row(data_idx).transpose() - mean_k_minus_1) / static_cast<double>(k + 1);

                // update the covariance
                for (eigen_idx_t d1 = 0; d1 < dimensions; d1++) {
                    double d1_diff = input_data(data_idx, d1) - mean_k_minus_1(d1);
                    for(eigen_idx_t d2 = d1; d2 < dimensions; d2++) {
                        double addition = (k / static_cast<double>(k+1)) * d1_diff
                                        * (input_data(data_idx, d2) - mean_k_minus_1(d2));
                        cov(d1, d2) += addition;
                        cov(d2, d1) = cov(d1, d2); // could make this more efficient by only doing it at the end
                    }
                }

                // We have only calcualted the upper triangular portion of the answer. Copy into the lower triangular bit
                for (eigen_idx_t d1 = 0; d1 < dimensions; d1++) { 
                    for (eigen_idx_t d2 = (d1 + 1); d2 < dimensions; d2++) {
                        cov(d2, d1) = cov(d1, d2);
                    }
                }
            }
        }

        inline void fit_params_inaccurate(const mtx<T> & input_data) {
            check_data_dimensionality(input_data);
            datapoint_idx_t num_input_datapoints = input_data.rows();

            initialise_params();

            // Two pass algorithm - calculate mean first. Use eigen to sum down
            // each column
            for (eigen_idx_t d = 0; d < dimensions; d++) {
                mean(d) = input_data.col(d).sum();
            }
            mean /= num_input_datapoints;


            for (eigen_idx_t i = 0; i < num_input_datapoints; i++) {
                const vec<T> data_point = input_data.row(i);
                for (eigen_idx_t d1 = 0; d1 < dimensions; d1++) {
                    for (eigen_idx_t d2 = 0; d2 < dimensions; d2++) {
                        cov(d1, d2) += (data_point(d1) - mean(d1)) * (data_point(d2) - mean(d2));
                    }
                }
            }

            // Unbiased estimator, because we don't know the population mean, only the sample
            // mean. Maybe I should have paid more attention in stats lessons, but I still don't
            // know why this works
            cov /= (num_input_datapoints - 1);
        }

        template<typename T1>
        friend std::ostream& operator<< (std::ostream& stream, const MultiDimGaussianX<T1>& mdg) {
            stream << "[mean[" << mdg.mean.transpose() << "]:cov[";
            for (eigen_idx_t r = 0; r < mdg.dimensions; r++) {
                stream << mdg.cov.row(r);
                if (r != (mdg.dimensions - 1)) { // if we have more rows to go..
                    stream << "; ";
                }
            }
            stream << "]]";
            return stream;
        }

#ifdef GARF_PYTHON_BINDINGS_ENABLE
        // Makes copes of the relevant objects
        inline PyObject* get_mean_numpy() const {
            return eigen_to_numpy_copy<T>(mean);
        }

        inline PyObject* get_cov_numpy() const {
            return eigen_to_numpy_copy<T>(cov);
        }
#endif

#ifdef GARF_SERIALIZE_ENABLE
    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
#endif
    };
}}

#endif
