import React, { useState } from "react";

export const enum SimulateTab {
    General = "General info",
    Instruction = "Instructions",
    Pressure = "Resources",
    Timeline = "Timeline",
}

interface SimulateToolbarProps {
    onTabSwitch: (tab: SimulateTab) => void;
    defaultTab?: SimulateTab;
}

export const SimulateToolbar: React.FC<SimulateToolbarProps> = ({
    onTabSwitch,
    defaultTab = SimulateTab.General,
}) => {
    const [tab, setTab] = useState(defaultTab);

    const handleTab = (newTab: SimulateTab) => {
        setTab(newTab);
        onTabSwitch(newTab);
    };

    const tabs = [
        SimulateTab.General,
        SimulateTab.Instruction,
        SimulateTab.Pressure,
        SimulateTab.Timeline,
    ];

    return (
        <div className="flex items-center justify-between gap-3 px-4 h-12 bg-white border-b border-gray-200 flex-shrink-0">
            <div className="flex items-center gap-2">
                {tabs.map((i) => (
                    <div
                        className={`px-3 py-1 rounded cursor-pointer border-2 text-base overflow-hidden text-nowrap ${
                            i === tab
                                ? "bg-white border-blue-600 font-semibold"
                                : "bg-gray-200 border-transparent hover:bg-gray-300/70"
                        }`}
                        onClick={() => handleTab(i)}
                        key={i.toString()}
                    >
                        {i.toString()}
                    </div>
                ))}
            </div>
        </div>
    );
};
