/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#include "meka/graph.hpp"

#include <boost/config.hpp> // put this first to suppress some VC++ warnings

#include <iostream>
#include <iterator>
#include <algorithm>
#include <time.h>

#include <boost/utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/visitors.hpp>

using namespace std;
using namespace boost;

namespace meka {

  enum files_e {
    dax_h, yow_h, boz_h, zow_h, foo_cpp,
    foo_o, bar_cpp, bar_o, libfoobar_a,
    zig_cpp, zig_o, zag_cpp, zag_o,
    libzigzag_a, killerapp, N
  };

  const char* name[] = {
    "dax.h", "yow.h", "boz.h", "zow.h", "foo.cpp",
    "foo.o", "bar.cpp", "bar.o", "libfoobar.a",
    "zig.cpp", "zig.o", "zag.cpp", "zag.o",
    "libzigzag.a", "killerapp"
  };

  struct print_visitor : public boost::bfs_visitor< > {
    template< class Vertex, class Graph >
    void discover_vertex(Vertex v, Graph&) { std::cout << name[v] << " "; }
  };

  struct cycle_detector : public boost::dfs_visitor< > {
    cycle_detector(bool& has_cycle) : has_cycle(has_cycle) {}

    template< class Edge, class Graph >
    void back_edge(Edge, Graph&) { has_cycle = true; }

    bool& has_cycle;
  };

  int main(int, char*[]) {
    typedef pair< int, int > Edge;

    Edge              used_by[] = {
      Edge(dax_h, foo_cpp), Edge(dax_h, bar_cpp), Edge(dax_h, yow_h),
      Edge(yow_h, bar_cpp), Edge(yow_h, zag_cpp),
      Edge(boz_h, bar_cpp), Edge(boz_h, zig_cpp), Edge(boz_h, zag_cpp),
      Edge(zow_h, foo_cpp),
      Edge(foo_cpp, foo_o),
      Edge(foo_o, libfoobar_a),
      Edge(bar_cpp, bar_o),
      Edge(bar_o, libfoobar_a),
      Edge(libfoobar_a, libzigzag_a),
      Edge(zig_cpp, zig_o),
      Edge(zig_o, libzigzag_a),
      Edge(zag_cpp, zag_o),
      Edge(zag_o, libzigzag_a),
      Edge(libzigzag_a, killerapp)
    };
    const std::size_t nedges = sizeof(used_by) / sizeof(Edge);

    typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::bidirectionalS > graph_type;
    typedef boost::graph_traits< graph_type >::vertex_descriptor                     vertex_type;

    auto graph = graph_type {};

    // Determine ordering for a full recompilation
    // and the order with files that can be compiled in parallel
    {
      typedef std::list< vertex_type > MakeOrder;

      MakeOrder::iterator i;
      MakeOrder           make_order;

      boost::topological_sort(graph, std::front_inserter(make_order));
      std::cout << "make ordering: ";
      for (i = make_order.begin(); i != make_order.end(); ++i) {
        std::cout << name[*i] << " ";
      }
      std::cout << std::endl << std::endl;

      // Parallel compilation ordering
      std::vector< int > time(N, 0);
      for (i = make_order.begin(); i != make_order.end(); ++i) {
        // Walk through the in_edges an calculate the maximum time.
        if (boost::in_degree(*i, graph) > 0) {
          graph_type::in_edge_iterator j, j_end;
          int                          maxdist = 0;

          // Through the order from topological sort, we are sure that every
          // time we are using here is already initialized.
          for (boost::tie(j, j_end) = boost::in_edges(*i, graph); j != j_end; ++j) {
            maxdist = (std::max) (time[boost::source(*j, graph)], maxdist);
          }
          time[*i] = maxdist + 1;
        }
      }

      std::cout << "parallel make orderingraph, " << std::endl
                << "vertices with same group number can be made in parallel" << std::endl;
      {
        graph_traits< graph_type >::vertex_iterator i, iend;
        for (boost::tie(i, iend) = boost::vertices(graph); i != iend; ++i) {
          std::cout << "time_slot[" << name[*i] << "] = " << time[*i] << std::endl;
        }
      }

    }
    std::cout << std::endl;

    // if I change yow.h what files need to be re-made?
    {
      std::cout << "A change to yow.h will cause what to be re-made?" << std::endl;
      boost::breadth_first_search(graph, boost::vertex(yow_h, graph), boost::visitor(print_visitor {}));
      std::cout << std::endl;
    }
    std::cout << std::endl;

    // are there any cycles in the graph?
    {
      bool has_cycle = false;
      boost::depth_first_search(graph, boost::visitor(cycle_detector { has_cycle }));
      std::cout << "The graph has a cycle? " << has_cycle << std::endl;
    }
    std::cout << std::endl;

    // add a dependency going from bar.cpp to dax.h
    {
      std::cout << "adding edge bar_cpp -> dax_h" << std::endl;
      boost::add_edge(bar_cpp, dax_h, graph);
    }
    std::cout << std::endl;

    // are there any cycles in the graph?
    {
      bool has_cycle = false;
      boost::depth_first_search(graph, boost::visitor(cycle_detector { has_cycle }));
      std::cout << "The graph has a cycle now? " << has_cycle << std::endl;
    }

    return 0;
  } // main

}
