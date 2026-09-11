import React from "react";
import { Undo2 } from 'lucide-react';
import { EditorTab } from "./EditorTabs";

interface EditorTabsLockedProps {
    tabs: EditorTab[];
    activeId: number | null;
    onBack: () => void;
}

const EditorTabsLocked: React.FC<EditorTabsLockedProps> = ({
    tabs,
    activeId,
    onBack,
}) => {
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
                    >
                        {tab.name}
                    </div>
                ))}
            </div>
            <button
                className="flex flex-row gap-2 items-center px-3 py-1 text-base text-white bg-blue-600 rounded-md hover:bg-blue-700"
                onClick={onBack}
            >
                Back to editor
                <Undo2 size={"1rem"} />
            </button>
        </div>
    );
};

export default EditorTabsLocked;
