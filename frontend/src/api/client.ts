import type { Node } from "../components/GraphNode";
import type { InstructionInfo } from "../components/SimViewInstructions";
import type { TimelineEntry } from "../components/SimViewTimeline";

export interface GraphStartRequest {
    source: string;
    filename: string;
    type: "c";
    compiler: string;
    flags: string;
}

export interface GraphStartResponse {
    session: string;
    functions: string[];
}

export interface GraphResponse {
    nodes: Node[];
}

export interface SimulateStartRequest {
    cpu: string;
    instructions: string[];
}

export interface SimulateSession {
    session: string;
}

export interface GeneralResults {
    iterations: number;
    instructions: number;
    totalCycles: number;
    totalMicroOps: number;
    uOpsPerCycle: number;
    instructionsPerCycle: number;
    blockRThroughput: number;
}

export interface BriefInstructionInfo {
    uOps: number;
    latency: number;
    rThroughput: number;
}

export interface FullInstructionInfo extends BriefInstructionInfo {
    bytes: number[];
    opcode: string;
}

export class ApiError extends Error {
    constructor(
        public readonly status: number,
        message: string,
    ) {
        super(message);
        this.name = "ApiError";
    }
}

async function request<T>(url: string, init?: RequestInit): Promise<T> {
    const response = await fetch(url, {
        headers: { "Content-Type": "application/json" },
        ...init,
    });

    const text = await response.text();
    let body: unknown = null;

    try {
        body = text ? JSON.parse(text) : null;
    } catch {
        body = null;
    }

    if (!response.ok) {
        const message =
            body && typeof body === "object" && "error" in body
                ? String((body as { error: string }).error)
                : `Request failed with status ${response.status}`;
        throw new ApiError(response.status, message);
    }

    return body as T;
}

export function graphStart(payload: GraphStartRequest): Promise<GraphStartResponse> {
    return request("/api/graph/start", {
        method: "POST",
        body: JSON.stringify(payload),
    });
}

export function graphStartBinary(
    bytes: ArrayBuffer,
): Promise<GraphStartResponse> {
    return request("/api/graph/start-binary", {
        method: "POST",
        headers: { "Content-Type": "application/octet-stream" },
        body: bytes,
    });
}

export function getGraph(session: string, name: string): Promise<GraphResponse> {
    return request(`/api/graph?session=${encodeURIComponent(session)}&name=${encodeURIComponent(name)}`);
}

export function simulateStart(payload: SimulateStartRequest): Promise<SimulateSession> {
    return request("/api/simulate/start", {
        method: "POST",
        body: JSON.stringify(payload),
    });
}

export function endSimulate(session: string): Promise<unknown> {
    return request(`/api/simulate/end?session=${encodeURIComponent(session)}`);
}

export function getGeneralResults(session: string): Promise<GeneralResults> {
    return request(`/api/simulate/general?session=${encodeURIComponent(session)}`);
}

export function getInstructionInfos(session: string): Promise<InstructionInfo[]> {
    return request(`/api/simulate/instructions?session=${encodeURIComponent(session)}`);
}

export function getTimelineEntries(session: string): Promise<TimelineEntry[]> {
    return request(`/api/simulate/timeline?session=${encodeURIComponent(session)}`);
}

export function getInstructionBrief(
    cpu: string,
    instruction: string,
): Promise<BriefInstructionInfo> {
    return request("/api/instruction/brief", {
        method: "POST",
        body: JSON.stringify({ cpu, instruction }),
    });
}

export function getInstructionFull(
    cpu: string,
    instruction: string,
): Promise<FullInstructionInfo> {
    return request("/api/instruction/full", {
        method: "POST",
        body: JSON.stringify({ cpu, instruction }),
    });
}
