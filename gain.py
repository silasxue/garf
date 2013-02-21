#!/usr/bin/env ipython --pylab -i
import numpy as np
import garf
from sys import getrefcount

def foo():

    import ipdb; ipdb.set_trace()


    rf = garf.RegressionForest()
    rf.forest_options.max_num_trees = 10
    rf.tree_options.max_depth = 2

    import ipdb; ipdb.set_trace()
    # for i in xrange(10000):
    #   blah = rf.numpy_object_float()
    #   print "id = ", hex(id(blah)), "refcount = ", getrefcount(blah), "dtype = ", blah.dtype, "blah = ", blah

    #   blah = rf.numpy_object_double()
    #   print "id = ", hex(id(blah)), "refcount = ", getrefcount(blah), "dtype = ", blah.dtype, "blah = ", blah
    # rf.load_forest("test_serialize.forest")

    # t0 = rf.get_tree(0)

    num_datapoints = 100
    f1 = np.linspace(-3, 3, num_datapoints).reshape(num_datapoints, 1)
    f2 = np.linspace(1, -2, num_datapoints).reshape(num_datapoints, 1)
    features = np.hstack([f1, f2])
    labels = ((features[:, 0] ** 2) + (features[:, 1])).reshape(num_datapoints, 1)

    print "features:\n", features
    print "labels:\n",  labels

    features = np.asfortranarray(features)
    labels = np.asfortranarray(labels)

    rf.train(features, labels.astype(np.float64))

    pred_mu = np.zeros(labels.shape, dtype=labels.dtype)
    pred_mu[:, :] = 0
    pred_var = np.zeros(labels.shape, dtype=labels.dtype)
    pred_var[:, :] = -1

    leaf_indices = np.zeros((features.shape[0], rf.stats.num_trees), dtype=np.long)
    leaf_indices[:, :] = -2

    pred_mu, pred_var, leaf_indices = rf.predict(features, output_leaf_indices=True)

    # print "vals after:", pred_mu
    # print "var after:", pred_var
    # print "leaf_indices:", leaf_indices

    return rf, features, labels, pred_mu, pred_var, leaf_indices

if __name__ == "__main__":
    rf, features, labels, pred_mu, pred_var, leaf_indices = foo()