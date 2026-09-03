import React from "react";
import { Node, InstructionRegion } from "../types";

interface GraphNodeProps {
    node: Node;
    x: number;
    y: number;
    width: number;
    height: number;
    onInstructionHover: (
        region: InstructionRegion,
        e: React.MouseEvent,
    ) => void;
    onInstructionClick: (
        region: InstructionRegion,
        e: React.MouseEvent,
    ) => void;
    onSelect: (e: React.MouseEvent) => void;
    selected?: boolean;
}

const GraphNode: React.FC<GraphNodeProps> = ({
    node,
    x,
    y,
    width,
    height,
    onInstructionHover,
    onInstructionClick,
    onSelect,
    selected = false,
}) => {
    return (
        <div
            className={`absolute bg-white rounded-sm select-none cursor-pointer ${
                selected ? "border-2 border-blue-500" : "border border-black"
            }`}
            style={{ left: x, top: y, width, height }}
            onClick={(e) => {
                if (!e.ctrlKey) {
                    e.preventDefault();
                    onSelect(e);
                }
            }}
        >
            <div className="font-bold text-lg px-2 py-1 font-sans">
                {node.label}
            </div>
            <div className="px-2 py-1 font-mono text-base">
                {node.instructions.map((instr, idx) => {
                    const region: InstructionRegion = {
                        node: node.id,
                        instrIndex: idx,
                        instrText: instr,
                    };
                    return (
                        <div
                            key={idx}
                            className="hover:bg-gray-100 cursor-default rounded-sm px-1"
                            onMouseEnter={(e) => onInstructionHover(region, e)}
                            onMouseLeave={(e) =>
                                onInstructionHover(null as any, e as any)
                            }
                            onClick={(e) => {
                                if (e.ctrlKey) {
                                    e.preventDefault();
                                    onInstructionClick(region, e);
                                }
                            }}
                        >
                            {instr}
                        </div>
                    );
                })}
            </div>
        </div>
    );
};

export default GraphNode;
