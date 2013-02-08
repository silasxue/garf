#ifndef GARF_OPTIONS_HPP
#define GARF_OPTIONS_HPP

#ifdef GARF_SERIALIZE_ENABLE
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#endif

namespace garf {

    /* Options which work at the top level of training - how
      many trees to make, what data to give them etc */
    struct ForestOptions {
        tree_idx_t max_num_trees;
        bool bagging;
        ForestOptions() : max_num_trees(2), bagging(true) {}
#ifdef GARF_SERIALIZE_ENABLE
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & max_num_trees;
            ar & bagging;
        }
#endif
    };

    /* Options which are needed inside a tree - ie when to stop splitting.
      These are needed regardless of how we are doing the splitting */
    struct TreeOptions {
        depth_idx_t max_depth;
        datapoint_idx_t min_sample_count; // don't bother with a split if
        double min_variance;
        TreeOptions() : max_depth(2), min_sample_count(10), min_variance(0.00001) {}
#ifdef GARF_SERIALIZE_ENABLE
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & max_depth;
            ar & min_sample_count;
            ar & min_variance;
        }
#endif
    };

    /* Options for how to split the tree */
    struct SplitOptions {
        split_idx_t num_splits_to_try;
        split_idx_t threshes_per_split;
        SplitOptions() : num_splits_to_try(5), threshes_per_split(3) {}
#ifdef GARF_SERIALIZE_ENABLE
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & num_splits_to_try;
            ar & threshes_per_split;
        }
#endif
    };

    /* Options for how we do the prediction (early stopping at maximum depth for example) */
    struct PredictOptions {
        depth_idx_t maximum_depth;
        PredictOptions() : maximum_depth(100) {}
#ifdef GARF_SERIALIZE_ENABLE
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & maximum_depth;
        }
#endif
    };
}

#endif