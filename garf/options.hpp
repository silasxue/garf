#ifndef GARF_OPTIONS_HPP
#define GARF_OPTIONS_HPP

#ifdef GARF_SERIALIZE_ENABLE
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#endif

#include "types.hpp"

namespace garf {

    // Options which work at the top level of training - how
    //  many trees to make, what data to give them etc
    struct ForestOptions {
        tree_idx_t max_num_trees;
        bool bagging;
        ForestOptions() : max_num_trees(2), bagging(true) {}
#ifdef GARF_SERIALIZE_ENABLE
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
#endif
    };

    // Options which are needed inside a tree - ie when to stop splitting.
    // These are needed regardless of how we are doing the splitting
    struct TreeOptions {
        depth_idx_t max_depth;
        datapoint_idx_t min_sample_count; // don't bother with a split if below this
        double min_variance;
        TreeOptions() : max_depth(2), min_sample_count(10), min_variance(0.00001) {}
#ifdef GARF_SERIALIZE_ENABLE
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
#endif
    };

    // Options for how to split the tree
    struct SplitOptions {
        split_idx_t num_splits_to_try;
        split_idx_t threshes_per_split;
        bool properly_random;  // Turn this off to make the system deterministic, for testing etc
        datapoint_idx_t num_per_side_for_viable_split;

        SplitOptions() :
            num_splits_to_try(5), threshes_per_split(3), 
            properly_random(true), num_per_side_for_viable_split(5) {}
#ifdef GARF_SERIALIZE_ENABLE
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
#endif
    };

    // Options for how we do the prediction (early stopping at maximum depth for example)
    struct PredictOptions {
        depth_idx_t maximum_depth;
        PredictOptions() : maximum_depth(100) {}
#ifdef GARF_SERIALIZE_ENABLE
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
#endif
    };
}

#endif