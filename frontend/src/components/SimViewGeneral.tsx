import React from "react";

interface GeneralViewProps {
    iterations: number;
    instructions: number;
    totalCycles: number;
    totalMicroOps: number;
    uOpsPerCycle: number;
    instructionsPerCycle: number;
    blockRThroughput: number;
}

const formatRate = (value: number) =>
    Number.isInteger(value) ? String(value) : value.toFixed(3);

export const GeneralView: React.FC<GeneralViewProps> = ({
    iterations,
    instructions,
    totalCycles,
    totalMicroOps,
    uOpsPerCycle,
    instructionsPerCycle,
    blockRThroughput,
}) => {
    const rows: Array<{ label: string; value: number | string }> = [
        { label: "Iterations", value: iterations },
        { label: "Instructions", value: instructions },
        { label: "Total cycles", value: totalCycles },
        { label: "Total μOps", value: totalMicroOps },
        { label: "μOps / cycle", value: formatRate(uOpsPerCycle) },
        { label: "IPC", value: formatRate(instructionsPerCycle) },
        { label: "Block reciprocal throughput", value: blockRThroughput },
    ];

    return (
        <div className="w-[60%]">
            <table className="table-fixed border-collapse w-full">
                <colgroup>
                    <col className="w-[75%]" />
                    <col className="w-[25%]" />
                </colgroup>
                <tbody>
                    {rows.map((row) => (
                        <tr
                            key={row.label}
                            className="border-b border-[#eee] last:border-b-0"
                        >
                            <td className="py-2 text-lg font-semibold text-left">
                                {row.label}
                            </td>
                            <td className="py-2 text-lg text-center">
                                {row.value}
                            </td>
                        </tr>
                    ))}
                </tbody>
            </table>
        </div>
    );
};
