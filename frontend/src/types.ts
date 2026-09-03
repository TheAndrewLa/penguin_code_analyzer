export interface InstructionRegion {
    node: string;
    instrIndex: number;
    instrText: string;
}

export interface InstructionInfo {
    uOps: number;
    latency: number;
    rThroughput: number;
    mayLoad: boolean;
    mayStore: boolean;
    sideEffects: boolean;
    text: string;
}

export interface TimelineEntry {
    iteration: number;
    instrIndex: number;
    instrText: string;
    dispatchCycles: number[];
    execCycles: number[];
    execEndCycles: number[];
    retireCycles: number[];
    waitQueueCycles: number[];
    waitRetireCycles: number[];
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

export interface EditorTab {
    id: number;
    name: string;
    type: "c" | "cpp" | "asm" | "binary";
    content: string;
}
