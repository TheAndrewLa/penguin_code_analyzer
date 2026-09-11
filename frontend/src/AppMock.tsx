import { useState, useCallback, useRef } from "react";
import Header from "./components/Header";
import EditorTabsLocked from "./components/EditorTabsLocked";
import { Editor } from "./components/Editor";
import { GraphView } from "./components/GraphView";
import InstructionTooltip from "./components/InstructionTooltip";
import InstructionModal from "./components/InstructionModal";
import EditOptionsModal from "./components/EditOptionsDialog";
import { SimulateToolbar, SimulateTab } from "./components/SimulateToolbar";
import { TimelineView, TimelineEntry } from "./components/SimViewTimeline";
import { EditorTab, EditorTabs } from "./components/EditorTabs";
import { InstructionRegion } from "./components/GraphNode";
import { GeneralView } from "./components/SimViewGeneral";
import { InstructionInfoView } from "./components/SimViewInstructions";
import { ResourcePressureView } from "./components/SimViewResources";
import { GraphToolbar } from "./components/GraphToolbar";

import {
    nodes,
    defaultCppContent,
    generalInfo,
    instructionInfo,
    instructions,
    haswell,
    resourcePressure,
    timelineEntries,
    defaultAsmContent,
} from "./mock";

function AppMock() {
    const [editorTabs, setEditorTabs] = useState<EditorTab[]>([
        { id: 1, name: "main.cpp", type: "cpp", content: defaultCppContent },
    ]);
    const [activeEditorTabId, setActiveEditorTabId] = useState<number>(1);
    const nextEditorTabId = useRef(2);

    const [simulateTab, setSimulateTab] = useState<SimulateTab>(
        SimulateTab.General,
    );

    const [hoveredRegion, setHoveredRegion] =
        useState<InstructionRegion | null>(null);
    const [tooltipPos, setTooltipPos] = useState({ x: 0, y: 0 });
    const [modalRegion, setModalRegion] = useState<InstructionRegion | null>(
        null,
    );

    const compilers = ["gcc 13.2", "clang 22.0"];
    const [compiler, setCompiler] = useState(compilers[0]);
    const [compileFlags, setCompileFlags] = useState("");
    const [isCompileModalOpen, setCompileModalOpen] = useState(false);

    const handleAddEditorTab = useCallback(
        (name: string, type: "c" | "cpp" | "asm") => {
            const newTab: EditorTab = {
                id: nextEditorTabId.current++,
                name: name.concat(".", type),
                type: type,
                content: defaultCppContent,
            };
            setEditorTabs((prev) => [...prev, newTab]);
            setActiveEditorTabId(newTab.id);
        },
        [],
    );

    const handleCloseEditorTab = useCallback(
        (id: number) => {
            setEditorTabs((prev) => {
                if (prev.length <= 1) return prev;
                const idx = prev.findIndex((t) => t.id === id);
                if (idx === -1) return prev;
                const newTabs = [...prev];
                newTabs.splice(idx, 1);
                if (activeEditorTabId === id) {
                    const newActive = newTabs[Math.max(0, idx - 1)];
                    setActiveEditorTabId(newActive.id);
                }
                return newTabs;
            });
        },
        [activeEditorTabId],
    );

    const handleSelectEditorTab = useCallback((id: number) => {
        setActiveEditorTabId(id);
    }, []);

    const handleEditorChange = useCallback(
        (value: string) => {
            setEditorTabs((prev) =>
                prev.map((tab) =>
                    tab.id === activeEditorTabId
                        ? { ...tab, content: value }
                        : tab,
                ),
            );
        },
        [activeEditorTabId],
    );

    const activeEditorTab = editorTabs.find((t) => t.id === activeEditorTabId);

    const handleCompile = useCallback(() => {
        console.log(`Compiling with ${compiler}, flags: ${compileFlags}`);
    }, [compiler, compileFlags]);

    const handleSimulate = useCallback(() => {
        alert("Pipeline simulation (demo)");
    }, []);

    const handleSimulateTabSwitch = useCallback((tab: SimulateTab) => {
        setSimulateTab(tab);
    }, []);

    const handleHover = (
        region: InstructionRegion | null,
        x: number,
        y: number,
    ) => {
        setHoveredRegion(region);
        if (region) {
            setTooltipPos({ x, y });
        }
    };

    const getInstructionMetrics = (insn: string) => {
        const lower = insn.toLowerCase();
        if (lower.includes("test") || lower.includes("cmp"))
            return { lat: 1, tp: 0.25, uops: 1 };
        if (lower.includes("mov")) return { lat: 1, tp: 0.25, uops: 1 };
        if (lower.includes("lea")) return { lat: 1, tp: 0.5, uops: 1 };
        if (lower.includes("add") || lower.includes("sub"))
            return { lat: 1, tp: 0.25, uops: 1 };
        if (lower.includes("j")) return { lat: 1, tp: 0.5, uops: 1 };
        return { lat: 1, tp: 0.33, uops: 1 };
    };

    const getSimulateComponent = (tab: SimulateTab) => {
        switch (tab) {
            case SimulateTab.General: {
                return (
                    <GeneralView
                        iterations={generalInfo.iterations}
                        instructions={generalInfo.instructions}
                        totalCycles={generalInfo.totalCycles}
                        totalMicroOps={generalInfo.totalMicroOps}
                        microOpsPerCycle={generalInfo.microOpsPerCycle}
                        instructionsPerCycle={generalInfo.ipc}
                        blockThroughput={generalInfo.blockRThroughput}
                    />
                );
            }
            case SimulateTab.Instruction: {
                return (
                    <InstructionInfoView
                        instructionInfos={instructionInfo}
                        instructions={instructions}
                    />
                );
            }
            case SimulateTab.Pressure: {
                return (
                    <ResourcePressureView
                        architecture={haswell}
                        instructions={instructions}
                        resourcePressure={resourcePressure}
                    />
                );
            }
            case SimulateTab.Timeline: {
                return (
                    <TimelineView
                        entries={timelineEntries}
                        instructions={instructions}
                    />
                );
            }
        }
    };

    return (
        <div className="flex flex-col h-screen overflow-hidden">
            <Header onShowInfo={() => alert("Showing info!")} />
            <div className="flex flex-1 overflow-hidden">
                <div className="w-[42%] min-w-[380px] flex flex-col border-r border-base-300 bg-base-100">
                    <div className="flex items-center h-12 bg-base-200 border-b border-base-300 px-2">
                        <EditorTabs
                            tabs={editorTabs}
                            activeId={activeEditorTabId}
                            onSelect={handleSelectEditorTab}
                            onClose={handleCloseEditorTab}
                            onAdd={handleAddEditorTab}
                            compiler={compiler}
                            compilers={compilers}
                            onCompilerChange={setCompiler}
                            onCompile={handleCompile}
                            onOpenOptions={() => setCompileModalOpen(true)}
                        />

                        {/*<EditorTabsLocked
                            tabs={editorTabs}
                            activeId={activeEditorTabId}
                            onBack={() => {}}
                        />*/}
                    </div>
                    <div className="flex-1 overflow-auto p-0">
                        {activeEditorTab && (
                            <Editor
                                key={activeEditorTab.id}
                                value={activeEditorTab.content}
                                onChange={handleEditorChange}
                                language={
                                    activeEditorTab.type === "asm"
                                        ? "asm"
                                        : "cpp"
                                }
                            />

                            // <Editor
                            //     key={activeEditorTab.id}
                            //     value={defaultAsmContent}
                            //     language="asm"
                            //     readOnly={true}
                            // />
                        )}
                    </div>
                </div>

                <div className="flex-1 flex flex-col bg-base-100">
                    <GraphToolbar onSimulate={handleSimulate} />
                    <GraphView
                        nodes={nodes}
                        onHover={handleHover}
                        onClick={setModalRegion}
                    />

                    {/*<SimulateToolbar onTabSwitch={handleSimulateTabSwitch} />
                    {getSimulateComponent(simulateTab)}*/}
                </div>
            </div>

            {hoveredRegion && (
                <InstructionTooltip
                    metrics={getInstructionMetrics(hoveredRegion.instrText)}
                    x={tooltipPos.x + 15}
                    y={tooltipPos.y - 20}
                />
            )}

            <InstructionModal
                region={modalRegion}
                onClose={() => setModalRegion(null)}
            />

            <EditOptionsModal
                isOpen={isCompileModalOpen}
                onClose={() => setCompileModalOpen(false)}
                onAccept={(flags) => setCompileFlags(flags)}
                initialFlags={compileFlags}
            />
        </div>
    );
}

export default AppMock;
