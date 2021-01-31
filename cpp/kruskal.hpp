/*
 * Kruskal.h
 *
 * Created on: Feb 04, 2020
 *     Author: Hung Tran (hung@ae.cs.uni-frankfurt.de)
 */

#ifndef EM_CC_KRUSKAL_H
#define EM_CC_KRUSKAL_H

#include "robin_hood.h"
#include "defs.hpp"


namespace SemiExtMem {
  #define im_node_map robin_hood::unordered_map
  #define simple_node_map std::vector

  template<typename EdgeVectorType>
  class Kruskal {
  private:
      const EdgeVectorType &_edges;
      const size_t _min_index;
      const size_t _max_index;
      em_mapping& _components;

	  node_t _next_node;
	  im_node_map<node_t,node_t> _id_map;
	  simple_node_map<node_t> _reverse_map;
	  simple_node_map<node_t> _parent;
	  simple_node_map<uint8_t> _height;

  public:
	  static constexpr size_t MEMORY_OVERHEAD_FACTOR = 8;
      Kruskal(const EdgeVectorType& edges, size_t min_index, size_t max_index, em_mapping & components) :
        _edges(edges),
        _min_index(min_index),
        _max_index(max_index),
        _components(components),
        _next_node(0),
        _id_map(),
        _reverse_map(),
        _parent(),
        _height()
      {
          // go through edges
	      int index = 0;
          for (auto it=edges.begin()+_min_index; it!=edges.begin()+_max_index; it++) {
	          index++;
              const auto& edge = *it;
	          node_t u = use_map(edge.u);
	          node_t v = use_map(edge.v);
              if (op_union(u, v)) {
              }
          }

          // output component assignment
          for (node_t i=0; i<_reverse_map.size(); ++i) {
	          const node_t& u = _reverse_map[i];
	          const node_t& j = op_find(i);
	          const node_t& v = _reverse_map[j];
	          if (u != v) {
		          _components.push_back(edge_t(u, v));
	          }
          }
      }

  private:

	  inline node_t use_map(node_t u) {
		  auto lookup = _id_map.find(u);
		  if (lookup == _id_map.end()) {
			  _id_map[u] = _next_node;
			  _reverse_map.push_back(u);
			  _parent.push_back(_next_node);
			  _height.push_back(0);
			  _next_node++;
			  return _next_node-1;
		  } else {
			  return lookup->second;
		  }

	  }

      inline node_t op_find(node_t u) {
          // find root
          node_t root = u;
          while (_parent[root] != root) {
	          root = _parent[root];
          }

          // apply path compression
          node_t tmp;
          while (_parent[u] != u) {
              tmp = _parent[u];
              _parent[u] = root;
              u = tmp;
          }

          return root;
      }

      inline bool op_union(node_t u, node_t v) {
          const node_t root_u = op_find(u);
          const node_t root_v = op_find(v);

          // cycle detected
          if (root_u == root_v) return false;

          // attach smaller to bigger tree
          if (_height[root_u] < _height[root_v]) {
              _parent[root_u] = root_v;
          } else {
              _parent[root_v] = root_u;
          }

          // increment the height of the resulting tree if they are the same
          if (_height[root_u] == _height[root_v]) {
              ++_height[root_u];
          }

          return true;
      }
  };

}

template class SemiExtMem::Kruskal<em_edge_vector>;
template class SemiExtMem::Kruskal<im_edge_vector>;
using IMKruskal = SemiExtMem::Kruskal<im_edge_vector>;
using SEMKruskal = SemiExtMem::Kruskal<em_edge_vector>;

#endif //EM_CC_KRUSKAL_H
