import React from "react";

export interface InstructionRegion {
    node: string;
    instrIndex: number;
    instrText: string;
}

export interface Edge {
    to: string;
    fallthrough?: boolean;
    conditional?: boolean;
    taken?: boolean;
}

export interface Node {
    id: string;
    label: string;
    instructions: string[];
    edgesOut: Edge[];
}

const MAX_INSTRUCTION_CHARS = 25;

export const truncateInstruction = (text: string): string =>
    text.length > MAX_INSTRUCTION_CHARS
        ? `${text.slice(0, MAX_INSTRUCTION_CHARS)}…`
        : text;

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
    onNodeHover: (id: string | null) => void;
}

export const GraphNode: React.FC<GraphNodeProps> = ({
    node,
    x,
    y,
    width,
    height,
    onInstructionHover,
    onInstructionClick,
    onSelect,
    selected = false,
    onNodeHover,
}) => {
    return (
        <div
            className={`absolute bg-white rounded-sm select-none cursor-pointer ${
                selected ? "border-2 border-ember" : "border border-black"
            }`}
            style={{ left: x, top: y, width, height }}
            onClick={(e) => {
                if (!e.ctrlKey) {
                    e.preventDefault();
                    onSelect(e);
                }
            }}
            onMouseEnter={() => onNodeHover?.(node.id)}
            onMouseLeave={() => onNodeHover?.(null)}
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
                            {truncateInstruction(instr)}
                        </div>
                    );
                })}
            </div>
        </div>
    );
};
