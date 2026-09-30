import React, { useRef, useState } from "react";
import * as Popover from "@radix-ui/react-popover";
import { Play, SlidersHorizontal, X, Plus, Upload } from "lucide-react";
import SelectField from "./primitives/SelectField";

export interface EditorTab {
    id: number;
    name: string;
    type: "c" | "binary";
    content: string;
}

interface EditorTabsProps {
    tabs: EditorTab[];
    activeId: number | null;
    onSelect: (id: number) => void;
    onClose: (id: number) => void;
    onAdd: (name: string) => void;
    onUpload: (file: File) => void;
    compiler: string;
    compilers: string[];
    onCompilerChange: (compiler: string) => void;
    onCompile: () => void;
    onOpenOptions: () => void;
    locked?: boolean;
    maxTabs?: number;
}

export const EditorTabs: React.FC<EditorTabsProps> = ({
    tabs,
    activeId,
    onSelect,
    onClose,
    onAdd,
    onUpload,
    compiler,
    compilers,
    onCompilerChange,
    onCompile,
    onOpenOptions,
    locked = false,
    maxTabs = 3,
}) => {
    const [isPopoverOpen, setIsPopoverOpen] = useState(false);
    const [newFileName, setNewFileName] = useState("main");
    const fileInputRef = useRef<HTMLInputElement>(null);

    const isLimitReached = tabs.length >= maxTabs;

    const onFileNameChanged = (value: string) => {
        const filtered = value.replace(/[^a-zA-Z0-9]/g, "");
        setNewFileName(filtered);
    };

    const onAddTab = () => {
        const trimmed = newFileName.trim();
        if (trimmed && !isLimitReached) {
            onAdd(trimmed);
            setNewFileName("main");
            setIsPopoverOpen(false);
        }
    };

    const onOpenChange = (open: boolean) => {
        if (open) {
            setNewFileName("main");
        }
        setIsPopoverOpen(open);
    };

    const onFileChosen = (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (file) onUpload(file);
        e.target.value = "";
    };

    return (
        <div className="flex items-center gap-1 overflow-x-auto flex-nowrap bg-white px-2 h-full w-full">
            <div className="flex items-center gap-1 overflow-x-auto flex-nowrap flex-1">
                {tabs.map((tab) => (
                    <div
                        key={tab.id}
                        className={`flex items-center gap-1 px-3 py-1 rounded cursor-pointer border-2 whitespace-nowrap ${
                            activeId === tab.id
                                ? "bg-white border-blue-600 font-semibold"
                                : "bg-gray-200 border-transparent hover:bg-gray-300/70"
                        }`}
                        onClick={() => onSelect(tab.id)}
                    >
                        <span className="truncate max-w-[120px] text-base">
                            {tab.name}
                        </span>
                        {tabs.length > 1 && (
                            <button
                                className="w-5 h-5 flex items-center justify-center text-gray-500 hover:text-gray-700"
                                onClick={(e) => {
                                    e.stopPropagation();
                                    onClose(tab.id);
                                }}
                            >
                                <X size={16} />
                            </button>
                        )}
                    </div>
                ))}

                <Popover.Root open={isPopoverOpen} onOpenChange={onOpenChange}>
                    <Popover.Trigger asChild>
                        <div className="flex flex-row gap-1">
                            <button
                                className={`w-7 h-7 flex items-center justify-center rounded ${
                                    isLimitReached
                                        ? "text-gray-400 cursor-not-allowed"
                                        : "hover:bg-gray-200"
                                }`}
                                disabled={isLimitReached}
                                title={
                                    isLimitReached
                                        ? `Maximum ${maxTabs} tabs allowed`
                                        : "Add new tab"
                                }
                            >
                                <Plus size={16} />
                            </button>
                        </div>
                    </Popover.Trigger>
                    <input
                        type="file"
                        accept=".o,.obj,.elf"
                        className="hidden"
                        ref={fileInputRef}
                        onChange={onFileChosen}
                    />
                    <button
                        className={`w-7 h-7 flex items-center justify-center rounded ${
                            isLimitReached
                                ? "text-gray-400 cursor-not-allowed"
                                : "hover:bg-gray-200"
                        }`}
                        onClick={(e) => {
                            e.preventDefault();
                            fileInputRef.current?.click();
                        }}
                        title="Upload binary object file"
                    >
                        <Upload size={16} />
                    </button>
                    <Popover.Portal>
                        <Popover.Content
                            className="bg-white border border-gray-300 rounded-md shadow-md p-3 z-50 w-40"
                            sideOffset={5}
                            align="start"
                        >
                            <div className="flex flex-col gap-3">
                                <div>
                                    <input
                                        type="text"
                                        value={newFileName}
                                        onChange={(e) =>
                                            onFileNameChanged(e.target.value)
                                        }
                                        className="w-full px-2 py-1 text-sm border border-gray-300 rounded focus:outline-none focus:ring-1 focus:ring-blue-500"
                                        placeholder="main"
                                        autoFocus
                                        onKeyDown={(e) => {
                                            if (e.key === "Enter") onAddTab();
                                            if (e.key === "Escape")
                                                setIsPopoverOpen(false);
                                        }}
                                    />
                                </div>
                                <button
                                    onClick={onAddTab}
                                    disabled={
                                        isLimitReached || !newFileName.trim()
                                    }
                                    className={`w-full py-1 text-sm font-medium text-white rounded ${
                                        isLimitReached || !newFileName.trim()
                                            ? "bg-gray-400 cursor-not-allowed"
                                            : "bg-blue-600 hover:bg-blue-700"
                                    }`}
                                >
                                    {isLimitReached ? "Limit reached" : "Add"}
                                </button>
                                {isLimitReached && (
                                    <p className="text-xs text-red-500 text-center">
                                        Maximum {maxTabs} tabs allowed
                                    </p>
                                )}
                            </div>
                            <Popover.Arrow className="fill-white" />
                        </Popover.Content>
                    </Popover.Portal>
                </Popover.Root>
            </div>

            <div className="flex gap-2">
                <SelectField
                    value={compiler}
                    onChange={onCompilerChange}
                    options={compilers}
                    disabled={locked}
                />

                <button
                    className="px-3 py-1 text-black bg-gray-200 rounded-md hover:bg-gray-300 disabled:bg-gray-100 disabled:cursor-not-allowed"
                    onClick={onOpenOptions}
                    disabled={locked}
                    title="Edit compile options"
                >
                    <SlidersHorizontal size="1rem" />
                </button>

                <button
                    className="px-3 py-1 text-white bg-blue-600 rounded-md hover:bg-blue-700 disabled:bg-gray-400 disabled:cursor-not-allowed"
                    onClick={onCompile}
                    disabled={locked}
                >
                    <Play size="1rem" />
                </button>
            </div>
        </div>
    );
};
