import React, {
    useRef,
    useMemo,
    useCallback,
    useEffect,
    useState,
} from "react";

import { Plus, Minus } from "lucide-react";

export interface TimelineEntry {
    iteration: number;
    instrIndex: number;
    dispatchCycles: number[];
    waitQueueCycles: number[];
    executeCycles: number[];
    executeEndCycles: number[];
    waitRetireCycles: number[];
    retireCycles: number[];
}

interface TimelineViewProps {
    entries: TimelineEntry[];
    instructions: string[];
}

export const TimelineView: React.FC<TimelineViewProps> = ({
    entries,
    instructions,
}) => {
    const containerRef = useRef<HTMLDivElement>(null);
    const isPanning = useRef(false);
    const startPan = useRef({ x: 0, y: 0 });

    const [scale, setScale] = useState(1);
    const [offset, setOffset] = useState({ x: 0, y: 0 });

    const cycles = useMemo(() => {
        return entries.reduce((acc, entry) => {
            const entryMax = Math.max(
                ...entry.dispatchCycles,
                ...entry.waitQueueCycles,
                ...entry.executeCycles,
                ...entry.executeEndCycles,
                ...entry.waitRetireCycles,
                ...entry.retireCycles,
            );
            return Math.max(acc, entryMax);
        }, 0);
    }, [entries]);

    const blockEntries = useMemo(() => {
        return entries.map((entry) => {
            const instruction = instructions[entry.instrIndex];
            const blocks = [];

            for (let c = 0; c < cycles; c++) {
                let label = "";
                let bgStyle = "bg-transparent border-b border-gray-400";

                if (entry.dispatchCycles.includes(c)) {
                    label = "D";
                    bgStyle = "bg-fuchsia-300 border border-black rounded-l-sm";
                } else if (entry.waitQueueCycles.includes(c)) {
                    label = "=";
                    bgStyle = "bg-orange-200 border border-black";
                } else if (entry.executeCycles.includes(c)) {
                    label = "e";
                    bgStyle = "bg-emerald-200 border border-black";
                } else if (entry.executeEndCycles.includes(c)) {
                    label = "E";
                    bgStyle = "bg-emerald-400 border border-black";
                } else if (entry.waitRetireCycles.includes(c)) {
                    label = "-";
                    bgStyle = "bg-yellow-200 border border-black";
                } else if (entry.retireCycles.includes(c)) {
                    label = "R";
                    bgStyle = "bg-indigo-300 border border-black rounded-r-sm";
                }

                blocks.push({ label, style: bgStyle });
            }

            return {
                ...entry,
                instruction,
                blocks,
            };
        });
    }, [entries, instructions, cycles]);

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

    return (
        <div
            ref={containerRef}
            className="flex-1 relative overflow-hidden cursor-grab"
            style={{
                backgroundImage: `
                linear-gradient(rgba(200, 200, 200, 0.3) 1px, transparent 1px),
                linear-gradient(90deg, rgba(200, 200, 200, 0.3) 1px, transparent 1px)`,
                backgroundSize: "30px 30px",
            }}
        >
            <div
                className="absolute top-0 left-0 select-none"
                style={{
                    transform: `translate(${offset.x}px, ${offset.y}px) scale(${scale})`,
                    transformOrigin: "0 0",
                }}
            >
                <div className="flex flex-col gap-1">
                    {blockEntries.map((entry, index) => (
                        <div
                            key={`line_${index}`}
                            className="flex flex-row gap-3 text-center"
                        >
                            <span className="self-stretch content-center w-40 min-w-40 px-2 overflow-hidden text-nowrap overflow-ellipsis font-mono text-sm bg-white border border-black rounded-sm">
                                {entry.instruction}
                            </span>
                            <div className="flex flex-row gap-1 text-center">
                                {entry.blocks.map((block, j) => (
                                    <div
                                        key={`block_${j}`}
                                        className={`self-stretch content-center w-8 h-8 font-mono text-sm ${block.style}`}
                                    >
                                        {block.label}
                                    </div>
                                ))}
                            </div>
                        </div>
                    ))}
                </div>
            </div>

            <div className="absolute z-20 bottom-4 right-4 flex items-center gap-1 bg-white border border-gray-200 rounded-lg p-2 shadow-md">
                <button onClick={zoomOut} title="Zoom out">
                    <Minus />
                </button>
                <button className="font-mono min-w-[3rem]" onClick={resetView}>
                    {Math.round(scale * 100)}%
                </button>
                <button onClick={zoomIn} title="Zoom in">
                    <Plus />
                </button>
            </div>
        </div>
    );
};
