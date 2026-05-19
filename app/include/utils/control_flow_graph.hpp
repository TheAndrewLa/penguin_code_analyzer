#ifndef APP_UTILS_CONTROL_FLOW_GRAPH_HPP
#define APP_UTILS_CONTROL_FLOW_GRAPH_HPP

#include <concepts>
#include <cstddef>
#include <queue>
#include <ranges>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace analyzer::utils {

template <class T>
concept CFGNodeType = std::movable<T>;

template <CFGNodeType T> class ControlFlowGraph;

namespace details {
template <CFGNodeType T> class CFGIterator {
public:
  using Graph = ControlFlowGraph<T>;
  using NodeId = typename Graph::NodeId;

  using iterator_category = std::forward_iterator_tag;
  using value_type = std::pair<NodeId, const T &>;
  using difference_type = std::ptrdiff_t;

  CFGIterator() = default;

  CFGIterator(const Graph *graph, const std::vector<NodeId> *order,
              std::size_t position)
      : m_graph(graph), m_order(order), m_position(position) {}

  value_type operator*() const {
    const auto nodeId = (*m_order)[m_position];
    return {nodeId, m_graph->node(nodeId)};
  }

  CFGIterator &operator++() {
    ++m_position;
    return *this;
  }

  CFGIterator operator++(int) {
    auto copy = *this;
    ++(*this);
    return copy;
  }

  bool operator==(const CFGIterator &other) const = default;

private:
  const Graph *m_graph{nullptr};
  const std::vector<NodeId> *m_order{nullptr};
  std::size_t m_position{0};
};
} // namespace details

template <CFGNodeType T>
class ControlFlowGraphView
    : public std::ranges::view_interface<ControlFlowGraphView<T>> {
public:
  using Graph = ControlFlowGraph<T>;
  using NodeIndex = typename Graph::NodeIndex;

  ControlFlowGraphView() = delete;

  ControlFlowGraphView(const Graph *graph, const std::vector<NodeIndex> *order)
      : m_graph(graph), m_order(order) {}

  details::CFGIterator<T> begin() const { return {m_graph, m_order, 0}; }

  details::CFGIterator<T> end() const {
    return {m_graph, m_order, m_order->size()};
  }

  std::size_t size() const { return m_order->size(); }

  bool empty() const { return size() == 0; }

private:
  const Graph *m_graph{nullptr};
  const std::vector<NodeIndex> *m_order{nullptr};
};

template <CFGNodeType T> class ControlFlowGraph {
public:
  using NodeIndex = std::size_t;
  using View = ControlFlowGraphView<T>;

  template <class U>
    requires std::constructible_from<T, U &&>
  NodeIndex addNode(U &&node) {
    const auto newNodeId = m_nextNodeId;
    ++m_nextNodeId;
    m_nodes.emplace(newNodeId, T(std::forward<U>(node)));
    m_adjacencyList[newNodeId] = {};
    return newNodeId;
  }

  void addEdge(NodeIndex from, NodeIndex to) {
    if (!hasNode(from) || !hasNode(to)) {
      throw std::out_of_range{"Failed to add edge to control flow graph"};
    }
    m_adjacencyList[from].push_back(to);
  }

  std::size_t nodeCount() const { return m_nodes.size(); }

  View dfs(NodeIndex root) const {
    if (!hasNode(root)) {
      throw std::out_of_range{"Failed to run DFS on control flow graph"};
    }

    m_depthFirstOrder.clear();
    std::unordered_map<NodeIndex, bool> visited;
    std::vector<NodeIndex> stack{root};

    while (!stack.empty()) {
      const auto currentNode = stack.back();
      stack.pop_back();
      if (visited[currentNode]) {
        continue;
      }

      visited[currentNode] = true;
      m_depthFirstOrder.push_back(currentNode);

      const auto &nextNodes = successors(currentNode);
      for (auto it = nextNodes.rbegin(); it != nextNodes.rend(); ++it) {
        if (!visited[*it]) {
          stack.push_back(*it);
        }
      }
    }

    return View{this, &m_depthFirstOrder};
  }

  View bfs(NodeIndex root) const {
    if (!hasNode(root)) {
      throw std::out_of_range{"Failed to run BFS on control flow graph"};
    }

    m_breadthFirstOrder.clear();
    std::unordered_map<NodeIndex, bool> visited;
    std::queue<NodeIndex> queue;

    queue.push(root);
    visited[root] = true;

    while (!queue.empty()) {
      const auto currentNode = queue.front();
      queue.pop();
      m_breadthFirstOrder.push_back(currentNode);

      for (const auto nextNode : successors(currentNode)) {
        if (!visited[nextNode]) {
          visited[nextNode] = true;
          queue.push(nextNode);
        }
      }
    }

    return View{this, &m_breadthFirstOrder};
  }

  View topsort() const {
    m_topologicalOrder.clear();
    std::unordered_map<NodeIndex, std::size_t> indegree;

    for (const auto &[nodeId, _] : m_nodes) {
      indegree[nodeId] = 0;
    }

    for (const auto &[_, nextNodes] : m_adjacencyList) {
      for (const auto nextNode : nextNodes) {
        ++indegree[nextNode];
      }
    }

    std::queue<NodeIndex> queue;
    for (const auto &[nodeId, degree] : indegree) {
      if (degree == 0) {
        queue.push(nodeId);
      }
    }

    while (!queue.empty()) {
      const auto currentNode = queue.front();
      queue.pop();
      m_topologicalOrder.push_back(currentNode);

      for (const auto nextNode : successors(currentNode)) {
        --indegree[nextNode];
        if (indegree[nextNode] == 0) {
          queue.push(nextNode);
        }
      }
    }

    if (m_topologicalOrder.size() != m_nodes.size()) {
      throw std::runtime_error{"Control flow graph contains cycle"};
    }

    return View{this, &m_topologicalOrder};
  }

private:
  friend class details::CFGIterator<T>;

  bool hasNode(NodeIndex nodeId) const { return m_nodes.contains(nodeId); }

  const T &node(NodeIndex nodeId) const {
    if (!hasNode(nodeId)) {
      throw std::out_of_range{"Failed to access control flow graph node"};
    }
    return m_nodes.at(nodeId);
  }

  const std::vector<NodeIndex> &successors(NodeIndex nodeId) const {
    if (!hasNode(nodeId)) {
      throw std::out_of_range{"Failed to access control flow graph successors"};
    }
    return m_adjacencyList.at(nodeId);
  }

  std::unordered_map<NodeIndex, T> m_nodes;
  std::unordered_map<NodeIndex, std::vector<NodeIndex>> m_adjacencyList;
  NodeIndex m_nextNodeId{0};

  mutable std::vector<NodeIndex> m_depthFirstOrder;
  mutable std::vector<NodeIndex> m_breadthFirstOrder;
  mutable std::vector<NodeIndex> m_topologicalOrder;
};
} // namespace analyzer::utils

#endif
