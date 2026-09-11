import React from "react";

export interface ArchitectureInfo {
    name: string;
    resources: string[];
    dispatchWidth: number;
}

interface ResourceUsageViewProps {
    architecture: ArchitectureInfo;
    resourcePressure: number[][];
    instructions: string[];
}

export const ResourcePressureView: React.FC<ResourceUsageViewProps> = ({
    architecture,
    resourcePressure,
    instructions,
}) => {
    const colorFromUsage = (usage: number) => {
        const minUsage = 0.1;
        const maxUsage = 0.9;
        if (usage <= minUsage) return "#ffeaab";
        if (usage >= maxUsage) return "#ff4d4d";
        const t = (usage - minUsage) / (maxUsage - minUsage);
        const q = t * t * t;
        const r = 255;
        const g = Math.round(234 - 157 * q);
        const b = Math.round(171 - 94 * q);
        return `rgb(${r}, ${g}, ${b})`;
    };

    return (
        <div className="flex flex-col p-5 gap-5 overflow-auto">
            <div className="flex flex-col w-[60%] p-2 gap-2 border border-black rounded-sm">
                <span className="text-xl font-semibold">
                    {architecture.name}
                </span>
                <span className="text-lg">
                    Dispatch width: {architecture.dispatchWidth}
                </span>
                <div className="flex flex-row flex-wrap gap-2">
                    {architecture.resources.map((r) => (
                        <div className="p-1 text-base font-mono bg-gray-100 border border-gray-500">
                            {r}
                        </div>
                    ))}
                </div>
            </div>
            <span className="text-lg font-semibold">
                Resource usage by instruction
            </span>
            <div className="flex flex-col w-[60%] gap-5">
                {instructions.map((text, i) => (
                    <div className="flex flex-row items-center gap-2">
                        <span
                            key={`instruction_${i}`}
                            className="w-[30%] min-w-[30%] text-base align-middle font-mono overflow-hidden text-nowrap text-ellipsis"
                            title={text}
                        >
                            {text}
                        </span>
                        <div className="w-[70%] flex flex-row flex-wrap gap-1">
                            {resourcePressure[i].map((pressure, j) => {
                                if (pressure > 0.01) {
                                    const name = architecture.resources[j];
                                    return (
                                        <div
                                            className="grow-[1] p-1 text-sm text-center border border-black rounded-sm"
                                            style={{
                                                backgroundColor: `${colorFromUsage(pressure)}`,
                                            }}
                                            title={`Usage is ${pressure}`}
                                        >
                                            {name}
                                        </div>
                                    );
                                }
                            })}
                        </div>
                    </div>
                ))}
            </div>
        </div>
    );
};
