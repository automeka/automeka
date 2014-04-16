/**
 * @file
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
 */

#include "meka/graph.hpp"
#include "meka/utility.hpp"

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
    template< typename Vertex, typename Graph >
    void discover_vertex(Vertex&& v, Graph&&) { std::cout << name[v] << " "; }
  };

  struct cycle_detector : public boost::dfs_visitor< > {
    cycle_detector(bool& has_cycle) : has_cycle(has_cycle) {}

    template< typename Edge, typename Graph >
    void back_edge(Edge&&, Graph&&) { has_cycle = true; }

    bool& has_cycle;
  };

  extern "C" int main(int, char*[]) {
    typedef pair< size_t, size_t > edge_type;

    auto const edges = std::vector< edge_type > {
      { dax_h, foo_cpp }, { dax_h, bar_cpp }, { dax_h, yow_h },
      { yow_h, bar_cpp }, { yow_h, zag_cpp },
      { boz_h, bar_cpp }, { boz_h, zig_cpp }, { boz_h, zag_cpp },
      { zow_h, foo_cpp },
      { foo_cpp, foo_o },
      { foo_o, libfoobar_a },
      { bar_cpp, bar_o },
      { bar_o, libfoobar_a },
      { libfoobar_a, libzigzag_a },
      { zig_cpp, zig_o },
      { zig_o, libzigzag_a },
      { zag_cpp, zag_o },
      { zag_o, libzigzag_a },
      { libzigzag_a, killerapp }
    };

    typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::bidirectionalS > graph_type;
    typedef boost::graph_traits< graph_type >::vertex_descriptor                     vertex_type;

    auto graph = graph_type { std::begin(edges), std::end(edges), 14 };

    // Determine ordering for a full recompilation
    // and the order with files that can be compiled in parallel
    {
      auto make_order = std::vector< vertex_type > {};

      boost::topological_sort(graph, std::back_inserter(make_order));
      std::reverse(std::begin(make_order), std::end(make_order));

      std::cout << "make ordering: ";
      for (auto const& i : make_order) { std::cout << name[i] << " "; }
      std::cout << "\n";

      std::vector< size_t > node_times(N, 0);
      for (auto const& i : make_order) {
        if (boost::in_degree(i, graph) > 0) {
          auto const& range = boost::in_edges(i, graph);

          auto const& maxelt = std::max_element(
            std::begin(range), std::end(range),
            [&node_times, &graph](decltype(*range.first) const& a, decltype(*range.first) const& b) {
              return node_times[boost::source(a, graph)] < node_times[boost::source(b, graph)];
            });

          node_times[i] = node_times[boost::source(*maxelt, graph)] + 1;
        }
      }

      std::cout << "parallel make orderingraph, " << "\n"
                << "vertices with same group number can be made in parallel" << "\n";
      {
        for (auto const& i : boost::vertices(graph)) {
          std::cout << "time_slot[" << name[i] << "] = " << node_times[i] << "\n";
        }
      }

    }
    std::cout << "\n";

    // if I change yow.h what files need to be re-made?
    {
      std::cout << "A change to yow.h will cause what to be re-made?" << "\n";
      boost::breadth_first_search(graph, boost::vertex(yow_h, graph), boost::visitor(print_visitor {}));
      std::cout << "\n";
    }
    std::cout << "\n";

    // are there any cycles in the graph?
    {
      bool has_cycle = false;
      boost::depth_first_search(graph, boost::visitor(cycle_detector { has_cycle }));
      std::cout << "The graph has a cycle? " << has_cycle << "\n";
    }
    std::cout << "\n";

    // add a dependency going from bar.cpp to dax.h
    {
      std::cout << "adding edge bar_cpp -> dax_h" << "\n";
      boost::add_edge(bar_cpp, dax_h, graph);
    }
    std::cout << "\n";

    // are there any cycles in the graph?
    {
      bool has_cycle = false;
      boost::depth_first_search(graph, boost::visitor(cycle_detector { has_cycle }));
      std::cout << "The graph has a cycle now? " << has_cycle << "\n";
    }

    return 0;
  } // main

}
