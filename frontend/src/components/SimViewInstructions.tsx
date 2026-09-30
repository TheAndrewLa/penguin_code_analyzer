import React from "react";
import { Check, Minus, Plus } from "lucide-react";

export interface InstructionInfo {
    uOps: number;
    latency: number;
    rThroughput: number;
    mayLoad: boolean;
    mayStore: boolean;
    sideFx: boolean;
}

interface InstructionInfoViewProps {
    instructionInfos: InstructionInfo[];
    instructions: string[];
}

export const InstructionInfoView: React.FC<InstructionInfoViewProps> = ({
    instructionInfos,
    instructions,
}) => {
    const headers = [" ", "#uOps ", "Lat ", "RThr ", "ML ", "MS ", "SideFx "];
    const headersLong = [
        "",
        "Micro operations",
        "Latency",
        "Reciprocal throughput",
        "May load",
        "May store",
        "Has unmodeled side effect",
    ];

    return (
        <div className="overflow-auto text-base">
            <table className="table-fixed border-collapse w-full">
                <colgroup>
                    <col className="w-[30%]" />
                    <col className="w-[10%]" />
                    <col className="w-[10%]" />
                    <col className="w-[15%]" />
                    <col className="w-[10%]" />
                    <col className="w-[10%]" />
                    <col className="w-[15%]" />
                </colgroup>
                <thead>
                    <tr className="border-b-2 border-[#ccc]">
                        {headers.map((h, i) => (
                            <th
                                key={h}
                                className="py-2 pr-3 font-semibold text-left"
                                title={headersLong[i]}
                            >
                                {h}
                            </th>
                        ))}
                    </tr>
                </thead>
                <tbody className="text-base">
                    {instructionInfos.map((info, i) => (
                        <tr
                            key={i}
                            className="border-b border-[#eee] last:border-b-0"
                        >
                            <td
                                className="py-1 pr-3 font-mono truncate"
                                title={instructions[i]}
                            >
                                {instructions[i]}
                            </td>
                            <td className="py-1 pr-3">{info.uOps}</td>
                            <td className="py-1 pr-3">{info.latency}</td>
                            <td className="py-1 pr-3">
                                {info.rThroughput.toFixed(2)}
                            </td>
                            <td className="py-1 pr-3">
                                {info.mayLoad ? (
                                    <Check size={"1rem"} />
                                ) : (
                                    <Minus size={"1rem"} />
                                )}
                            </td>
                            <td className="py-1 pr-3">
                                {info.mayStore ? (
                                    <Check size={"1rem"} />
                                ) : (
                                    <Minus size={"1rem"} />
                                )}
                            </td>
                            <td className="py-1 pr-3">
                                {info.sideFx ? (
                                    <Plus size={"1rem"} />
                                ) : (
                                    <Minus size={"1rem"} />
                                )}
                            </td>
                        </tr>
                    ))}
                </tbody>
            </table>
        </div>
    );
};
