import React from "react";

interface InstructionTooltipProps {
    metrics: { lat: number; tp: number; uops: number };
    x: number;
    y: number;
}

const InstructionTooltip: React.FC<InstructionTooltipProps> = ({ metrics, x, y }) => {
    return (
        <div
            className="fixed pointer-events-none border border-base-300 rounded-lg shadow-lg p-3 font-mono text-sm z-50 bg-white"
            style={{ left: x, top: y }}
        >
            <div>Latency: {metrics.lat}</div>
            <div>Throughput: {metrics.tp}</div>
            <div>uOps: {metrics.uops}</div>
            <div className="text-xs text-neutral/60 mt-1">
                Ctrl+Click — Intel manual
            </div>
        </div>
    );
};

export default InstructionTooltip;
