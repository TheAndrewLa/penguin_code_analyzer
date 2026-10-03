import React from "react";
import SelectField from "./primitives/SelectField";

interface GraphToolbarProps {
    functions: string[];
    selectedFunction: string;
    onFunctionChange: (func: string) => void;
    uArch: string;
    uArchOptions: string[];
    onUArchChange: (uArch: string) => void;
    onSimulate: () => void;
    graphLoaded: boolean;
    blockSelected: boolean;
    simulating: boolean;
}

export const GraphToolbar: React.FC<GraphToolbarProps> = ({
    functions,
    selectedFunction,
    onFunctionChange,
    uArch,
    uArchOptions,
    onUArchChange,
    onSimulate,
    graphLoaded,
    blockSelected,
    simulating,
}) => {
    return (
        <div className="flex items-center justify-between gap-3 px-4 h-12 bg-white border-b border-gray-200 flex-shrink-0">
            <div className="flex items-center gap-2">
                {graphLoaded && (
                    <div className="flex gap-2">
                        <span className="text-base font-medium text-gray-900">Select function</span>
                        <SelectField
                            value={selectedFunction}
                            onChange={onFunctionChange}
                            options={functions}
                        />
                    </div>
                )}
            </div>
            <div className="flex gap-2">
                <SelectField
                    value={uArch}
                    onChange={onUArchChange}
                    options={uArchOptions}
                    disabled={!graphLoaded}
                />
                <button
                    className="px-3 py-1 text-base font-medium text-gray-900 bg-ember rounded-md hover:brightness-95 disabled:bg-gray-400 disabled:cursor-not-allowed"
                    onClick={onSimulate}
                    disabled={!graphLoaded || !blockSelected || simulating}
                >
                    Simulate
                </button>
            </div>
        </div>
    );
};
