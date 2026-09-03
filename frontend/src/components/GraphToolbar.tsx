import React, { useState } from "react";
import SelectField from "./primitives/SelectField";

interface GraphToolbarProps {
    onSimulate: () => void;
}

const GraphToolbar: React.FC<GraphToolbarProps> = ({ onSimulate }) => {
    const [uArch, setUArch] = useState("Haswell");
    const [func, setFunc] = useState("fib(int)");

    const uArchOptions = ["Haswell", "Rocket Lake", "AMD Zen"];
    const funcOptions = ["fib(int)", "concat(char*)"];

    return (
        <div className="flex items-center justify-between gap-3 px-4 h-12 bg-white border-b border-gray-200 flex-shrink-0">
            <div className="flex items-center gap-2">
                <SelectField
                    value={func}
                    onChange={setFunc}
                    options={funcOptions}
                />
            </div>
            <div className="flex gap-2">
                <SelectField
                    value={uArch}
                    onChange={setUArch}
                    options={uArchOptions}
                />
                <button
                    className="px-3 py-1 text-base font-medium text-white bg-blue-600 rounded-md hover:bg-blue-700"
                    onClick={onSimulate}
                >
                    Simulate
                </button>
            </div>
        </div>
    );
};

export default GraphToolbar;
