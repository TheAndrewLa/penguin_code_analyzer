import React, {
    useRef,
    useMemo,
    useCallback,
    useEffect,
    useState,
} from "react";

import { Plus, Minus } from "lucide-react";
import { Node, InstructionRegion, GraphNode, truncateInstruction } from "./GraphNode";

interface GraphViewProps {
    nodes: Node[];
    onHover: (
        region: InstructionRegion | null,
        mouseX: number,
        mouseY: number,
    ) => void;
    onClick: (region: InstructionRegion) => void;
    selectedNodeId?: string | null;
    onSelectNode?: (id: string | null) => void;
}

// Measure text width using a hidden span
const measureTextWidth = (text: string, font: string): number => {
    const span = document.createElement("span");
    span.style.font = font;
    span.style.position = "absolute";
    span.style.visibility = "hidden";
    span.style.whiteSpace = "nowrap";
    span.textContent = text;
    document.body.appendChild(span);
    const width = span.offsetWidth;
    document.body.removeChild(span);
    return width;
};

enum EdgeType {
    Adjacent = "adjacent",
    Back = "back",
    LongForward = "longForward",
}

interface LayoutNode extends Node {
    x: number;
    y: number;
    width: number;
    height: number;
    level: number;
}

interface Port {
    x: number;
    y: number;
}

interface EdgePorts {
    sourcePort: Port;
    targetPort: Port;
    pathD: string;
}

export const GraphView: React.FC<GraphViewProps> = ({
    nodes,
    onHover,
    onClick,
    selectedNodeId = null,
    onSelectNode,
}) => {
    const containerRef = useRef<HTMLDivElement>(null);
    const isPanning = useRef(false);
    const startPan = useRef({ x: 0, y: 0 });

    // Internal state for zoom and pan
    const [scale, setScale] = useState(1);
    const [offset, setOffset] = useState({ x: 0, y: 0 });

    // Main layout computation
    const layout = useMemo(() => {
        if (!nodes || nodes.length === 0) {
            return { nodeData: [], edgePaths: [] };
        }

        // --- 1. Build graph; nodes[0] is the root ---
        const nodeMap: Record<string, Node> = {};
        nodes.forEach((n) => (nodeMap[n.id] = n));

        const rootId = nodes[0].id;

        // --- 2. Keep only blocks reachable from the root ---
        const reachable: Set<string> = new Set();
        const bfsQueue: string[] = [rootId];
        reachable.add(rootId);

        while (bfsQueue.length > 0) {
            const currentId = bfsQueue.shift()!;
            const currentNode = nodeMap[currentId];
            if (!currentNode) continue;
            currentNode.edgesOut.forEach((edge) => {
                const targetId = edge.to;
                if (nodeMap[targetId] && !reachable.has(targetId)) {
                    reachable.add(targetId);
                    bfsQueue.push(targetId);
                }
            });
        }

        const nodeList = nodes.filter((n) => reachable.has(n.id));

        // --- 3. BFS to assign levels (shortest distance from root) ---
        const level: Record<string, number> = {};
        const visited: Record<string, boolean> = {};

        level[rootId] = 0;
        visited[rootId] = true;
        bfsQueue.length = 0;
        bfsQueue.push(rootId);

        while (bfsQueue.length > 0) {
            const currentId = bfsQueue.shift()!;
            const currentNode = nodeMap[currentId];
            if (!currentNode) continue;
            currentNode.edgesOut.forEach((edge) => {
                const targetId = edge.to;
                if (
                    !visited[targetId] &&
                    nodeMap[targetId] &&
                    reachable.has(targetId)
                ) {
                    visited[targetId] = true;
                    level[targetId] = level[currentId] + 1;
                    bfsQueue.push(targetId);
                }
            });
        }

        // Assign level 0 to any unvisited nodes (should not happen)
        nodeList.forEach((n) => {
            if (level[n.id] === undefined) level[n.id] = 0;
        });

        // --- 3. Group nodes by level ---
        const levelGroups: Record<number, Node[]> = {};

        nodeList.forEach((n) => {
            const l = level[n.id] ?? 0;
            if (!levelGroups[l]) levelGroups[l] = [];
            levelGroups[l].push(n);
        });

        const maxLevel = Math.max(...Object.keys(levelGroups).map(Number));

        // --- 4. Compute node dimensions ---
        const font = '16px "IBM Plex Mono", monospace';
        const labelFont = "bold 18px Inter, sans-serif";
        const paddingX = 20;
        const paddingY = 12;
        const lineHeight = 24;
        const minWidth = 200;

        const nodeDimensions: Record<
            string,
            { width: number; height: number }
        > = {};
        nodeList.forEach((n) => {
            let maxInstrWidth = 0;
            n.instructions.forEach((instr) => {
                const w = measureTextWidth(truncateInstruction(instr), font);
                if (w > maxInstrWidth) maxInstrWidth = w;
            });
            const labelWidth = measureTextWidth(n.label, labelFont);
            const width = Math.max(
                minWidth,
                maxInstrWidth + paddingX * 2,
                labelWidth + paddingX * 2,
            );
            const height = 40 + n.instructions.length * lineHeight + paddingY;
            nodeDimensions[n.id] = { width, height };
        });

        // --- 5. Order nodes within each level (e.g., by id) ---
        const orderedLevels: Record<number, string[]> = {};
        Object.keys(levelGroups).forEach((lvl) => {
            const l = Number(lvl);
            const nodeList = levelGroups[l];
            nodeList.sort((a, b) => a.id.localeCompare(b.id));
            orderedLevels[l] = nodeList.map((n) => n.id);
        });

        // --- 6. Calculate horizontal layout: center each level ---
        const gapX = 20;
        const verticalGap = 50; // fixed gap between levels
        const topMargin = 20;

        // Calculate width of each level
        const levelWidths: Record<number, number> = {};
        Object.keys(orderedLevels).forEach((lvl) => {
            const l = Number(lvl);
            const ids = orderedLevels[l];
            let totalWidth = 0;
            ids.forEach((id, idx) => {
                totalWidth += nodeDimensions[id].width;
                if (idx < ids.length - 1) totalWidth += gapX;
            });
            levelWidths[l] = totalWidth;
        });

        const maxLevelWidth = Math.max(...Object.values(levelWidths), 0);

        // --- 7. Allocate margins for external edges (back / long forward) ---
        let backEdgeCount = 0;
        let longForwardCount = 0;
        const edges: { from: string; to: string; type: EdgeType }[] = [];

        nodeList.forEach((n) => {
            n.edgesOut.forEach((e) => {
                const fromLevel = level[n.id] ?? 0;
                const toLevel = level[e.to] ?? 0;
                let type: EdgeType;
                if (toLevel === fromLevel + 1) {
                    type = EdgeType.Adjacent;
                } else if (toLevel > fromLevel + 1) {
                    type = EdgeType.LongForward;
                    longForwardCount++;
                } else {
                    type = EdgeType.Back;
                    backEdgeCount++;
                }
                edges.push({ from: n.id, to: e.to, type });
            });
        });

        const laneWidth = 20;
        const lanePadding = 10;

        const leftMargin = Math.max(
            80,
            longForwardCount * (laneWidth + lanePadding) + 20,
        );
        const rightMargin = Math.max(
            80,
            backEdgeCount * (laneWidth + lanePadding) + 20,
        );

        // --- 8. Compute vertical positions (level Y) with fixed gap ---
        const levelY: Record<number, number> = {};
        const levelBottom: Record<number, number> = {}; // bottom of each level (max y + height)

        for (let l = 0; l <= maxLevel; l++) {
            if (l === 0) {
                levelY[l] = topMargin;
            } else {
                const prevBottom = levelBottom[l - 1];
                levelY[l] = prevBottom + verticalGap;
            }
            // compute max bottom for this level
            const ids = orderedLevels[l] || [];
            let maxH = 0;
            ids.forEach((id) => {
                const h = nodeDimensions[id].height;
                if (h > maxH) maxH = h;
            });
            levelBottom[l] = levelY[l] + maxH;
        }

        // --- 9. Assign node positions (x and y) ---
        const nodePositions: Record<string, { x: number; y: number }> = {};

        Object.keys(orderedLevels).forEach((lvl) => {
            const l = Number(lvl);
            const ids = orderedLevels[l];
            const totalWidth = levelWidths[l];
            const offsetX = (maxLevelWidth - totalWidth) / 2;
            let curX = leftMargin + offsetX;
            const y = levelY[l];

            ids.forEach((id) => {
                const w = nodeDimensions[id].width;
                nodePositions[id] = { x: curX, y };
                curX += w + gapX;
            });
        });

        // --- 10. Assign ports for each edge (evenly distributed) ---
        const sidePorts: Record<
            string,
            {
                bottom: string[];
                top: string[];
                right: string[];
                left: string[];
            }
        > = {};

        nodeList.forEach((n) => {
            sidePorts[n.id] = { bottom: [], top: [], right: [], left: [] };
        });

        edges.forEach((edge) => {
            const fromId = edge.from;
            const toId = edge.to;
            // Source side: adjacent and back use bottom; longForward use left
            let srcSide: "bottom" | "left" =
                edge.type === EdgeType.Adjacent || edge.type === EdgeType.Back
                    ? "bottom"
                    : "left";
            // Target side: adjacent use top; back also use top; longForward use left
            let tgtSide: "top" | "left";
            if (
                edge.type === EdgeType.Adjacent ||
                edge.type === EdgeType.Back
            ) {
                tgtSide = "top";
            } else {
                tgtSide = "left";
            }

            const edgeKey = fromId + "->" + toId;
            sidePorts[fromId][srcSide].push(edgeKey);
            sidePorts[toId][tgtSide].push(edgeKey);
        });

        // Sort each side list for deterministic order
        nodeList.forEach((n) => {
            const id = n.id;
            Object.keys(sidePorts[id]).forEach((side) => {
                sidePorts[id][
                    side as keyof (typeof sidePorts)[typeof id]
                ].sort();
            });
        });

        // Function to get port coordinates with even distribution
        const getPortCoords = (
            nodeId: string,
            side: "bottom" | "top" | "right" | "left",
            index: number,
            total: number,
        ): Port => {
            const pos = nodePositions[nodeId];
            const dim = nodeDimensions[nodeId];
            const x = pos.x;
            const y = pos.y;
            const w = dim.width;
            const h = dim.height;

            if (side === "bottom" || side === "top") {
                const step = total > 0 ? w / (total + 1) : w / 2;
                const offsetX = step * (index + 1);
                const portX = x + offsetX;
                const portY = side === "bottom" ? y + h : y;
                return { x: portX, y: portY };
            } else {
                // left or right
                const step = total > 0 ? h / (total + 1) : h / 2;
                const offsetY = step * (index + 1);
                const portY = y + offsetY;
                const portX = side === "right" ? x + w : x;
                return { x: portX, y: portY };
            }
        };

        // Map each edge to its source and target port
        const edgePorts: Record<string, EdgePorts> = {};
        edges.forEach((edge) => {
            const fromId = edge.from;
            const toId = edge.to;
            let srcSide: "bottom" | "left" =
                edge.type === EdgeType.Adjacent || edge.type === EdgeType.Back
                    ? "bottom"
                    : "left";
            let tgtSide: "top" | "left";
            if (
                edge.type === EdgeType.Adjacent ||
                edge.type === EdgeType.Back
            ) {
                tgtSide = "top";
            } else {
                tgtSide = "left";
            }

            const srcList = sidePorts[fromId][srcSide];
            const tgtList = sidePorts[toId][tgtSide];
            const edgeKey = fromId + "->" + toId;
            const srcIndex = srcList.indexOf(edgeKey);
            const tgtIndex = tgtList.indexOf(edgeKey);
            const srcTotal = srcList.length;
            const tgtTotal = tgtList.length;

            const srcPort = getPortCoords(fromId, srcSide, srcIndex, srcTotal);
            const tgtPort = getPortCoords(toId, tgtSide, tgtIndex, tgtTotal);

            edgePorts[edgeKey] = {
                sourcePort: srcPort,
                targetPort: tgtPort,
                pathD: "",
            };
        });

        // --- 11. Build edge paths with arrows ---
        const edgePaths: { d: string; color: string; markerEnd: string }[] = [];

        // Helper to get color and marker id
        const getEdgeStyle = (
            fromId: string,
            toId: string,
        ): { color: string; markerId: string } => {
            const fromNode = nodeMap[fromId];
            if (!fromNode) return { color: "#000000", markerId: "arrow-black" };
            const edgeDef = fromNode.edgesOut.find((e) => e.to === toId);
            if (!edgeDef) return { color: "#000000", markerId: "arrow-black" };
            if (edgeDef.conditional && edgeDef.taken) {
                return { color: "#2e7d32", markerId: "arrow-green" };
            }
            if (edgeDef.conditional) {
                return { color: "#992017", markerId: "arrow-red" };
            }
            return { color: "#000000", markerId: "arrow-black" };
        };

        // Compute overall max X for right lane placement
        const maxNodeX = Math.max(
            ...nodeList.map((n) => nodePositions[n.id]?.x || 0),
        );
        const maxNodeWidth = Math.max(
            ...nodeList.map((n) => nodeDimensions[n.id]?.width || 0),
        );
        const maxX = maxNodeX + maxNodeWidth;

        // Back edges (right side)
        const backEdges = edges.filter((e) => e.type === EdgeType.Back);
        backEdges.sort((a, b) => {
            const la = level[a.from] ?? 0;
            const lb = level[b.from] ?? 0;
            if (la !== lb) return la - lb;
            return a.from.localeCompare(b.from);
        });

        backEdges.forEach((edge, idx) => {
            const key = edge.from + "->" + edge.to;
            const ports = edgePorts[key];
            if (!ports) return;
            const srcLevel = level[edge.from] ?? 0;
            const bottomY = levelBottom[srcLevel]; // bottom of the source level
            const laneX = maxX + rightMargin + idx * (laneWidth + lanePadding);
            const srcPort = ports.sourcePort;
            const tgtPort = ports.targetPort; // now target is top side

            // Path: down from source port to bottomY+15, right to lane, up to target port y (top), left to target port
            const d =
                `M ${srcPort.x} ${srcPort.y} ` +
                `L ${srcPort.x} ${bottomY + 15} ` +
                `L ${laneX} ${bottomY + 15} ` +
                `L ${laneX} ${tgtPort.y - 15} ` +
                `L ${tgtPort.x} ${tgtPort.y - 15}` +
                `L ${tgtPort.x} ${tgtPort.y}`;

            const style = getEdgeStyle(edge.from, edge.to);
            edgePaths.push({
                d,
                color: style.color,
                markerEnd: `url(#${style.markerId})`,
            });
        });

        // Long forward edges (left side)
        const longEdges = edges.filter((e) => e.type === EdgeType.LongForward);
        longEdges.sort((a, b) => {
            const la = level[a.from] ?? 0;
            const lb = level[b.from] ?? 0;
            if (la !== lb) return la - lb;
            return a.from.localeCompare(b.from);
        });

        longEdges.forEach((edge, idx) => {
            const key = edge.from + "->" + edge.to;
            const ports = edgePorts[key];
            if (!ports) return;
            const laneX = -leftMargin - idx * (laneWidth + lanePadding);
            const srcPort = ports.sourcePort;
            const tgtPort = ports.targetPort;
            const d =
                `M ${srcPort.x} ${srcPort.y} ` +
                `L ${laneX} ${srcPort.y} ` +
                `L ${laneX} ${tgtPort.y} ` +
                `L ${tgtPort.x} ${tgtPort.y}`;
            const style = getEdgeStyle(edge.from, edge.to);
            edgePaths.push({
                d,
                color: style.color,
                markerEnd: `url(#${style.markerId})`,
            });
        });

        // Adjacent edges (straight vertical)
        const adjacentEdges = edges.filter((e) => e.type === EdgeType.Adjacent);
        adjacentEdges.forEach((edge) => {
            const key = edge.from + "->" + edge.to;
            const ports = edgePorts[key];
            if (!ports) return;
            const d = `M ${ports.sourcePort.x} ${ports.sourcePort.y} ` +
                `L ${ports.sourcePort.x} ${ports.sourcePort.y + 10} ` +
                `L ${ports.targetPort.x} ${ports.sourcePort.y + 10}` +
                `L ${ports.targetPort.x} ${ports.targetPort.y - 10}` +
                `L ${ports.targetPort.x} ${ports.targetPort.y}`;
            const style = getEdgeStyle(edge.from, edge.to);
            edgePaths.push({
                d,
                color: style.color,
                markerEnd: `url(#${style.markerId})`,
            });
        });

        // --- 12. Prepare node data for rendering ---
        const nodeData: LayoutNode[] = nodeList.map((n) => ({
            ...n,
            x: nodePositions[n.id]?.x || 0,
            y: nodePositions[n.id]?.y || 0,
            width: nodeDimensions[n.id]?.width || 0,
            height: nodeDimensions[n.id]?.height || 0,
            level: level[n.id] ?? 0,
        }));

        return { nodeData, edgePaths };
    }, [nodes]);

    const handleWheel = useCallback(
        (e: WheelEvent) => {
            e.preventDefault();
            const delta = e.deltaY > 0 ? 0.9 : 1.1;
            const newScale = Math.min(Math.max(scale * delta, 0.3), 3.0);
            const rect = containerRef.current?.getBoundingClientRect();
            if (!rect) return;
            const mouseX = e.clientX - rect.left;
            const mouseY = e.clientY - rect.top;
            const worldX = (mouseX - offset.x) / scale;
            const worldY = (mouseY - offset.y) / scale;
            setScale(newScale);
            setOffset({
                x: mouseX - worldX * newScale,
                y: mouseY - worldY * newScale,
            });
        },
        [scale, offset],
    );

    const handleMouseDown = useCallback(
        (e: MouseEvent) => {
            isPanning.current = true;
            startPan.current = {
                x: e.clientX - offset.x,
                y: e.clientY - offset.y,
            };
            if (containerRef.current)
                containerRef.current.style.cursor = "grabbing";
        },
        [offset],
    );

    const handleMouseMove = useCallback((e: MouseEvent) => {
        if (isPanning.current) {
            setOffset({
                x: e.clientX - startPan.current.x,
                y: e.clientY - startPan.current.y,
            });
        }
    }, []);

    const handleMouseUp = useCallback(() => {
        isPanning.current = false;
        if (containerRef.current) containerRef.current.style.cursor = "grab";
    }, []);

    useEffect(() => {
        const container = containerRef.current;
        if (!container) return;
        container.addEventListener("wheel", handleWheel, { passive: false });
        container.addEventListener("mousedown", handleMouseDown);
        window.addEventListener("mousemove", handleMouseMove);
        window.addEventListener("mouseup", handleMouseUp);
        return () => {
            container.removeEventListener("wheel", handleWheel);
            window.removeEventListener("mousemove", handleMouseMove);
            window.removeEventListener("mouseup", handleMouseUp);
        };
    }, [handleWheel, handleMouseDown, handleMouseMove, handleMouseUp]);

    const resetView = useCallback(() => {
        setScale(1);
        setOffset({ x: 0, y: 0 });
    }, []);

    const zoomIn = useCallback(
        () => setScale(Math.min(scale * 1.2, 3.0)),
        [scale],
    );

    const zoomOut = useCallback(
        () => setScale(Math.max(scale / 1.2, 0.3)),
        [scale],
    );

    const handleNodeSelect = useCallback(
        (id: string) => {
            onSelectNode?.(selectedNodeId === id ? null : id);
        },
        [selectedNodeId, onSelectNode],
    );

    const handleInstructionHover = useCallback(
        (region: InstructionRegion | null, e: React.MouseEvent) => {
            if (region) {
                onHover(region, e.clientX, e.clientY);
            } else {
                onHover(null, 0, 0);
            }
        },
        [onHover],
    );

    const handleInstructionClick = useCallback(
        (region: InstructionRegion) => {
            onClick(region);
        },
        [onClick],
    );

    return (
        <div
            ref={containerRef}
            className="flex-1 relative overflow-hidden bg-base-100 cursor-grab"
            style={{
                backgroundImage: `
                linear-gradient(rgba(200, 200, 200, 0.3) 1px, transparent 1px),
                linear-gradient(90deg, rgba(200, 200, 200, 0.3) 1px, transparent 1px)`,
                backgroundSize: "30px 30px",
            }}
        >
            <div
                className="absolute top-0 left-0 w-full h-full"
                style={{
                    transform: `translate(${offset.x}px, ${offset.y}px) scale(${scale})`,
                    transformOrigin: "0 0",
                }}
            >
                {/* SVG for edges */}
                <svg className="absolute top-0 left-0 w-full h-full pointer-events-none overflow-visible">
                    <defs>
                        <marker
                            id="arrow-black"
                            markerWidth="6"
                            markerHeight="6"
                            refX="5"
                            refY="3"
                            orient="auto"
                        >
                            <path d="M 0 0 L 6 3 L 0 6 z" fill="#000000" />
                        </marker>
                        <marker
                            id="arrow-green"
                            markerWidth="6"
                            markerHeight="6"
                            refX="5"
                            refY="3"
                            orient="auto"
                        >
                            <path d="M 0 0 L 6 3 L 0 6 z" fill="#2e7d32" />
                        </marker>
                        <marker
                            id="arrow-red"
                            markerWidth="6"
                            markerHeight="6"
                            refX="5"
                            refY="3"
                            orient="auto"
                        >
                            <path d="M 0 0 L 6 3 L 0 6 z" fill="#992017" />
                        </marker>
                    </defs>
                    {layout.edgePaths.map((edge, idx) => (
                        <path
                            key={idx}
                            d={edge.d}
                            stroke={edge.color}
                            strokeWidth={1.5}
                            fill="none"
                            markerEnd={edge.markerEnd}
                        />
                    ))}
                </svg>

                {/* Nodes */}
                {layout.nodeData.map((node) => (
                    <GraphNode
                        key={node.id}
                        node={node}
                        x={node.x}
                        y={node.y}
                        width={node.width}
                        height={node.height}
                        onInstructionHover={handleInstructionHover}
                        onInstructionClick={handleInstructionClick}
                        selected={selectedNodeId === node.id}
                        onSelect={() => handleNodeSelect(node.id)}
                    />
                ))}
            </div>

            <div className="absolute z-20 bottom-4 right-4 flex items-center gap-1 bg-white border border-gray-200 rounded-lg p-2 shadow-md">
                <button
                    className="btn btn-ghost btn-sm btn-square"
                    onClick={zoomOut}
                    title="Zoom out"
                >
                    <Minus />
                </button>
                <button
                    className="btn btn-ghost btn-sm font-mono min-w-[3rem]"
                    onClick={resetView}
                >
                    {Math.round(scale * 100)}%
                </button>
                <button
                    className="btn btn-ghost btn-sm btn-square"
                    onClick={zoomIn}
                    title="Zoom in"
                >
                    <Plus />
                </button>
            </div>
        </div>
    );
};
