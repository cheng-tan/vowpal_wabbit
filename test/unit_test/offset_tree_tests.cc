#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <utility>
#include "offset_tree.h"
using namespace LEARNER;
using namespace std;

namespace VW{ namespace offset_tree {
  std::ostream& operator<<(std::ostream& os, const tree_node& node) {
    os << "{" << node.id << "," << node.left_id << "," << node.right_id << ", " << (node.is_leaf ? "true" : "false") << "}";
    return os;
  }
}}

struct reduction_test_harness {
  reduction_test_harness():
  _curr_idx(0){}

  void set_predict_response(const vector<pair<float, float>>& predictions) { _predictions = predictions; }

  void test_predict(single_learner& base, example& ec) {
    ec.pred.a_s.clear();
    const auto curr_pred = _predictions[_curr_idx++];
    ec.pred.a_s.push_back(ACTION_SCORE::action_score{ 0,curr_pred.first});
    ec.pred.a_s.push_back(ACTION_SCORE::action_score{ 1,curr_pred.second});
  }

  void test_learn(single_learner& base, example& ec) { 
    throw "Not yet implemented";
  }

  static void predict(reduction_test_harness& test_reduction, single_learner& base, example& ec) {
    test_reduction.test_predict(base,ec);
  }

  static void learn(reduction_test_harness& test_reduction, single_learner& base, example& ec) {
    test_reduction.test_learn(base,ec);
  };
private:
  vector<pair<float, float>> _predictions;
  int _curr_idx;
};

using test_learner_t = learner<reduction_test_harness, example>;
using predictions_t = vector<pair<float, float>>;
using scores_t = std::vector<float>;

void predict_test_helper(const predictions_t& base_reduction_predictions, const scores_t& expected_scores);
BOOST_AUTO_TEST_CASE(offset_tree_predict) {
  // 0 node tree
  predict_test_helper({ {} }, {}); 
  // 1 node tree  
  predict_test_helper({ {} }, { 1.0f }); 
  // 2 node trees
  predict_test_helper({ {.2f,.8f} }, { .2f,.8f });
  predict_test_helper({ {.1f,.9f} }, { .1f,.9f });
  predict_test_helper({ {0.0f,1.0f} }, { 0.0f,1.0f });
  // 3 node trees
  predict_test_helper(
    { {.9f,.1f}, {.9f,.1f} },
    { .9f*.9f,.9f*.1f,.1f });
  predict_test_helper(
    { {.2f,.8f}, {.2f,.8f} },
    { .2f*.2f,.2f*.8f,.8f });
  // 4 node tree
  predict_test_helper(
    { {.9f,.1f}, {.9f,.1f}, {.9f,.1f} },
    { .9f*.9f,.9f*.1f,.1f*.9f,.1f*.1f });
  // 5 node tree
  predict_test_helper(
    { {.9f,.1f}, {.9f,.1f}, {.9f,.1f}, {.9f,.1f} },
    { .9f*.9f*.9f,.9f*.9f*.1f,.9f*.1f*.9f,.9f*.1f*.1f,.1f});
}

BOOST_AUTO_TEST_CASE(build_min_depth_tree_11) {
  VW::offset_tree::min_depth_binary_tree tree;
  tree.build_tree(11);
  std::vector<VW::offset_tree::tree_node> expected = {
      { 0,0,0,true},
      { 1,0,0,true},
      { 2,0,0,true},
      { 3,0,0,true},
      { 4,0,0,true},
      { 5,0,0,true},
      { 6,0,0,true},
      { 7,0,0,true},
      { 8,0,0,true},
      { 9,0,0,true},
      { 10,0,0,true},
      { 11,0,1,false},
      { 12,2,3,false},
      { 13,4,5,false},
      { 14,6,7,false},
      { 15,8,9,false},
      { 16,11,12,false},
      { 17,13,14,false},
      { 18,15,10,false},
      { 19,16,17,false},
      { 20,19,18,false}
  };
  BOOST_CHECK_EQUAL_COLLECTIONS(tree.nodes.begin(), tree.nodes.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(build_min_depth_tree_4) {
  VW::offset_tree::min_depth_binary_tree tree;
  tree.build_tree(4);
  std::vector<VW::offset_tree::tree_node> expected = {
      { 0,0,0,true},
      { 1,0,0,true},
      { 2,0,0,true},
      { 3,0,0,true},
      { 4,0,1,false},
      { 5,2,3,false},
      { 6,4,5,false}
  };
  BOOST_CHECK_EQUAL_COLLECTIONS(tree.nodes.begin(), tree.nodes.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(build_min_depth_tree_3) {
  VW::offset_tree::min_depth_binary_tree tree;
  tree.build_tree(3);
  std::vector<VW::offset_tree::tree_node> expected = {
      { 0,0,0,true},
      { 1,0,0,true},
      { 2,0,0,true},
      { 3,0,1,false},
      { 4,3,2,false}
  };
  BOOST_CHECK_EQUAL_COLLECTIONS(tree.nodes.begin(), tree.nodes.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(build_min_depth_tree_1) {
  VW::offset_tree::min_depth_binary_tree tree;
  tree.build_tree(1);
  std::vector<VW::offset_tree::tree_node> expected = {
      { 0,0,0,true}
  };
  BOOST_CHECK_EQUAL_COLLECTIONS(tree.nodes.begin(), tree.nodes.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(build_min_depth_tree_too_big) {
  VW::offset_tree::min_depth_binary_tree tree;
  // Throws vw_exception when unable to allocate enough memory to build tree
  BOOST_CHECK_THROW(tree.build_tree(INT_MAX),VW::vw_exception);
}

test_learner_t* get_test_harness_reduction(const predictions_t& base_reduction_predictions) {
  // Setup a test harness base reduction
  auto test_harness = scoped_calloc_or_throw<reduction_test_harness>();
  test_harness->set_predict_response(base_reduction_predictions);
  auto& test_learner = init_learner(
    test_harness,                    // Data structure passed by vw_framework into test_harness predict/learn calls
    reduction_test_harness::learn,   // test_harness learn
    reduction_test_harness::predict, // test_harness predict
    1                                // Number of regressors in test_harness (not used)
  );                                // Create a learner using the base reduction.
  return &test_learner;
}

void predict_test_helper(const predictions_t& base_reduction_predictions, const scores_t& expected_scores) {
  const auto test_base = get_test_harness_reduction(base_reduction_predictions);
  VW::offset_tree::offset_tree tree;
  tree.init(expected_scores.size());
  example ec;
  ec.pred.a_s = v_init<ACTION_SCORE::action_score>();
  auto& ret_val = tree.predict(*as_singleline(test_base), ec);
  BOOST_CHECK_EQUAL_COLLECTIONS(ret_val.begin(), ret_val.end(), expected_scores.begin(), expected_scores.end());
  destroy_free<test_learner_t>(test_base);
}
