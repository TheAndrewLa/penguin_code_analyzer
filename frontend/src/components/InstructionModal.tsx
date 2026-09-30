import React from "react";
import * as Dialog from "@radix-ui/react-dialog";
import { X } from "lucide-react";
import { InstructionRegion } from "@/types";
import type { FullInstructionInfo } from "../api/client";

interface InstructionModalProps {
    region: InstructionRegion | null;
    info?: FullInstructionInfo | null;
    onClose: () => void;
}

const InstructionModal: React.FC<InstructionModalProps> = ({
    region,
    info,
    onClose,
}) => {
    if (!region) return null;

    const bytesHex = info ? info.bytes.map((byte) => byte.toString(16).padStart(2, "0")).join(" ") : "";

    return (
        <Dialog.Root
            open={!!region}
            onOpenChange={(open) => !open && onClose()}
        >
            <Dialog.Portal>
                <Dialog.Overlay className="fixed inset-0 bg-black/50 z-50" />
                <Dialog.Content className="fixed top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 bg-white rounded-lg p-6 shadow-xl w-[90vw] max-w-lg z-50">
                    <Dialog.Title className="text-lg font-bold">
                        Intel® 64 and IA-32 Architectures
                    </Dialog.Title>
                    <div className="mt-4">
                        <strong>Instruction reference</strong>
                        <pre className="mt-2 bg-gray-100 p-4 rounded-lg overflow-x-auto font-mono text-sm">
                            {region.instrText}
                            {"\n"}
                            {info ? (
                                <>
                                    {"\n"}
                                    {`Latency: ${info.latency}`}
                                    {"\n"}
                                    {`uOps: ${info.uOps}`}
                                    {"\n"}
                                    {`Reciprocal throughput: ${info.rThroughput}`}
                                    {"\n"}
                                    {`Opcode: ${info.opcode}`}
                                    {"\n"}
                                    {`Bytes: ${bytesHex}`}
                                </>
                            ) : (
                                <>
                                    {"\n"}
                                    Intel® 64 and IA-32 Architectures Software
                                    Developer’s Manual
                                    {"\n"}Vol. 2A 3-XXX
                                    {"\n"}Opcode: ...
                                    {"\n"}Description: ...
                                    {"\n"}Flags affected: OF,SF,ZF,AF,PF,CF
                                </>
                            )}
                        </pre>
                    </div>
                    <Dialog.Close asChild>
                        <button className="absolute top-2 right-2 p-1 text-gray-400 hover:text-gray-600">
                            <X strokeWidth={1.25} />
                        </button>
                    </Dialog.Close>
                </Dialog.Content>
            </Dialog.Portal>
        </Dialog.Root>
    );
};

export default InstructionModal;