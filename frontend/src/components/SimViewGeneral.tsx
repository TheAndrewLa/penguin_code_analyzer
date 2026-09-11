import React from "react";

interface GeneralViewProps {
    iterations: number;
    instructions: number;
    totalCycles: number;
    totalMicroOps: number;
    microOpsPerCycle: number;
    instructionsPerCycle: number;
    blockThroughput: number;
}

export const GeneralView: React.FC<GeneralViewProps> = ({
    iterations,
    instructions,
    totalCycles,
    totalMicroOps,
    microOpsPerCycle,
    instructionsPerCycle,
    blockThroughput,
}) => {
    const rows: Array<{ label: string; value: number | string }> = [
        { label: "Iterations", value: iterations },
        { label: "Instructions", value: instructions },
        { label: "Total cycles", value: totalCycles },
        { label: "Total μOps", value: totalMicroOps },
        { label: "μOps / cycle", value: microOpsPerCycle },
        { label: "IPC", value: instructionsPerCycle },
        { label: "Block reciprocal throughput", value: blockThroughput },
    ];

    return (
        <div className="w-[60%] p-5">
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
