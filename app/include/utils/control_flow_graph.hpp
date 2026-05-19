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
concept ControlFlowGraphNodeType = std::movable<T>;

namespace details {
class ControlFlowGraphViewIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  ControlFlowGraphViewIterator() = default;
  ControlFlowGraphViewIterator(const std::vector<std::size_t> *order,
                               std::size_t position)
      : m_order(order), m_position(position) {}

  value_type operator*() const { return (*m_order)[m_position]; }

  ControlFlowGraphViewIterator &operator++() {
    ++m_position;
    return *this;
  }

  ControlFlowGraphViewIterator operator++(int) {
    auto copy = *this;
    ++(*this);
    return copy;
  }

  bool operator==(const ControlFlowGraphViewIterator &other) const = default;

private:
  const std::vector<std::size_t> *m_order{nullptr};
  std::size_t m_position{0};
};
} // namespace details

class ControlFlowGraphView
    : public std::ranges::view_interface<ControlFlowGraphView> {
public:
  using iterator = details::ControlFlowGraphViewIterator;

  ControlFlowGraphView() = default;
  explicit ControlFlowGraphView(const std::vector<std::size_t> *order)
      : m_order(order) {}

  iterator begin() const { return iterator{m_order, 0}; }

  iterator end() const {
    return iterator{m_order, m_order == nullptr ? 0 : m_order->size()};
  }

  std::size_t size() const { return m_order == nullptr ? 0 : m_order->size(); }

  bool empty() const { return size() == 0; }

private:
  const std::vector<std::size_t> *m_order{nullptr};
};

template <ControlFlowGraphNodeType T> class ControlFlowGraph {
public:
  using NodeId = std::size_t;

  template <class U>
    requires std::constructible_from<T, U &&>
  NodeId addNode(U &&node) {
    const auto newNodeId = m_nextNodeId;
    ++m_nextNodeId;
    m_nodes.emplace(newNodeId, T(std::forward<U>(node)));
    m_adjacencyList[newNodeId] = {};
    return newNodeId;
  }

  void addEdge(NodeId from, NodeId to) {
    if (!contains(from) || !contains(to)) {
      throw std::out_of_range{"Failed to add edge to control flow graph"};
    }
    m_adjacencyList[from].push_back(to);
  }

  bool contains(NodeId nodeId) const { return m_nodes.contains(nodeId); }

  std::size_t nodeCount() const { return m_nodes.size(); }

  const T &node(NodeId nodeId) const {
    if (!contains(nodeId)) {
      throw std::out_of_range{"Failed to access control flow graph node"};
    }
    return m_nodes.at(nodeId);
  }

  ControlFlowGraphView dfs(NodeId root) const {
    if (!contains(root)) {
      throw std::out_of_range{"Failed to run DFS on control flow graph"};
    }

    m_depthFirstOrder.clear();
    std::unordered_map<NodeId, bool> visited;
    std::vector<NodeId> stack{root};

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

    return ControlFlowGraphView{&m_depthFirstOrder};
  }

  ControlFlowGraphView bfs(NodeId root) const {
    if (!contains(root)) {
      throw std::out_of_range{"Failed to run BFS on control flow graph"};
    }

    m_breadthFirstOrder.clear();
    std::unordered_map<NodeId, bool> visited;
    std::queue<NodeId> queue;

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

    return ControlFlowGraphView{&m_breadthFirstOrder};
  }

private:
  const std::vector<NodeId> &successors(NodeId nodeId) const {
    if (!contains(nodeId)) {
      throw std::out_of_range{"Failed to access control flow graph successors"};
    }
    return m_adjacencyList.at(nodeId);
  }

  std::unordered_map<NodeId, T> m_nodes;
  std::unordered_map<NodeId, std::vector<NodeId>> m_adjacencyList;
  NodeId m_nextNodeId{0};

  mutable std::vector<NodeId> m_depthFirstOrder;
  mutable std::vector<NodeId> m_breadthFirstOrder;
};

} // namespace analyzer::utils

#endif
