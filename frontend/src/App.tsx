import { useCallback, useEffect, useRef, useState } from "react";
import Header from "./components/Header";
import { Editor } from "./components/Editor";
import { GraphView } from "./components/GraphView";
import InstructionTooltip from "./components/InstructionTooltip";
import InstructionModal from "./components/InstructionModal";
import EditOptionsModal from "./components/EditOptionsDialog";
import { SimulateToolbar, SimulateTab } from "./components/SimulateToolbar";
import { TimelineView, TimelineEntry } from "./components/SimViewTimeline";
import { EditorTab, EditorTabs } from "./components/EditorTabs";
import EditorTabsLocked from "./components/EditorTabsLocked";
import { InstructionRegion, Node } from "./components/GraphNode";
import { GeneralView } from "./components/SimViewGeneral";
import {
    InstructionInfo,
    InstructionInfoView,
} from "./components/SimViewInstructions";
import { GraphToolbar } from "./components/GraphToolbar";
import {
    compilers,
    cpuByDisplayName,
    DEFAULT_COMPILER,
    uArchOptions,
} from "./config";
import {
    BriefInstructionInfo,
    FullInstructionInfo,
    GeneralResults,
    getGeneralResults,
    getGraph,
    getInstructionBrief,
    getInstructionFull,
    getInstructionInfos,
    getTimelineEntries,
    graphStart,
    graphStartBinary,
    simulateStart,
    endSimulate,
} from "./api/client";

interface SimulateResult {
    general: GeneralResults;
    instructions: string[];
    instructionInfos: InstructionInfo[];
    timelineEntries: TimelineEntry[];
}

const DEFAULT_C_CONTENT = `int fib(int n) {
    int prev = 1, current = 1;
    for (; n >= 0; --n) {
        int tmp = prev + current;
        prev = current;
        current = tmp;
    }
    return current;
}`;

function App() {
    const [editorTabs, setEditorTabs] = useState<EditorTab[]>([
        { id: 1, name: "main.c", type: "c", content: DEFAULT_C_CONTENT },
    ]);
    const [activeEditorTabId, setActiveEditorTabId] = useState<number>(1);
    const nextEditorTabId = useRef(2);

    const [compiler, setCompiler] = useState(DEFAULT_COMPILER);
    const [compileFlags, setCompileFlags] = useState("");
    const [isCompileModalOpen, setCompileModalOpen] = useState(false);
    const [isCompiling, setIsCompiling] = useState(false);
    const [compileError, setCompileError] = useState<string | null>(null);

    const [uArch, setUArch] = useState(uArchOptions[0]);

    const [graphSession, setGraphSession] = useState<string | null>(null);
    const [functions, setFunctions] = useState<string[]>([]);
    const [selectedFunction, setSelectedFunction] = useState("");
    const [nodes, setNodes] = useState<Node[]>([]);
    const [selectedBlockId, setSelectedBlockId] = useState<string | null>(null);

    const [simulateTab, setSimulateTab] = useState<SimulateTab>(
        SimulateTab.General,
    );
    const [simulateResult, setSimulateResult] = useState<SimulateResult | null>(
        null,
    );
    const [simulateSession, setSimulateSession] = useState<string | null>(null);
    const [simulateMode, setSimulateMode] = useState(false);
    const [isSimulating, setIsSimulating] = useState(false);
    const [simulateError, setSimulateError] = useState<string | null>(null);

    const hoverTimer = useRef<ReturnType<typeof setTimeout> | null>(null);
    const [hoveredRegion, setHoveredRegion] =
        useState<InstructionRegion | null>(null);

    const [tooltipPos, setTooltipPos] = useState({ x: 0, y: 0 });
    const [tooltipMetrics, setTooltipMetrics] =
        useState<BriefInstructionInfo | null>(null);

    const [modalRegion, setModalRegion] = useState<InstructionRegion | null>(
        null,
    );
    const [modalInfo, setModalInfo] = useState<FullInstructionInfo | null>(
        null,
    );

    const activeEditorTab = editorTabs.find((t) => t.id === activeEditorTabId);

    const cpu = cpuByDisplayName[uArch] ?? "haswell";

    useEffect(() => {
        return () => {
            if (hoverTimer.current) clearTimeout(hoverTimer.current);
        };
    }, []);

    const handleAddEditorTab = useCallback((name: string) => {
        const newTab: EditorTab = {
            id: nextEditorTabId.current++,
            name: name.concat(".c"),
            type: "c",
            content: DEFAULT_C_CONTENT,
        };
        setEditorTabs((prev) => [...prev, newTab]);
        setActiveEditorTabId(newTab.id);
    }, []);

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

    const loadGraph = useCallback(async (session: string, name: string) => {
        const result = await getGraph(session, name);
        setNodes(result.nodes);
    }, []);

    const handleCompile = useCallback(async () => {
        if (!activeEditorTab || activeEditorTab.type !== "c" || isCompiling)
            return;

        setIsCompiling(true);
        setCompileError(null);
        setSimulateResult(null);
        setSelectedBlockId(null);

        try {
            const result = await graphStart({
                source: activeEditorTab.content,
                filename: activeEditorTab.name,
                type: "c",
                compiler,
                flags: compileFlags,
            });

            setGraphSession(result.session);
            setFunctions(result.functions);
            setSimulateResult(null);

            if (result.functions.length > 0) {
                const first = result.functions[0];
                setSelectedFunction(first);
                await loadGraph(result.session, first);
            } else {
                setSelectedFunction("");
                setNodes([]);
            }
        } catch (err) {
            setCompileError(err instanceof Error ? err.message : String(err));
        } finally {
            setIsCompiling(false);
        }
    }, [activeEditorTab, isCompiling, compiler, compileFlags, loadGraph]);

    const handleUploadBinary = useCallback(
        async (file: File) => {
            setIsCompiling(true);
            setCompileError(null);
            setSimulateResult(null);
            setSelectedBlockId(null);

            try {
                const bytes = await file.arrayBuffer();
                const result = await graphStartBinary(bytes);

                setGraphSession(result.session);
                setFunctions(result.functions);
                setSimulateResult(null);

                if (result.functions.length > 0) {
                    const first = result.functions[0];
                    setSelectedFunction(first);
                    await loadGraph(result.session, first);
                } else {
                    setSelectedFunction("");
                    setNodes([]);
                }

                const existing = editorTabs.find(
                    (tab) => tab.type === "binary",
                );
                let binaryTabId: number;

                if (existing) {
                    binaryTabId = existing.id;
                    setEditorTabs((prev) =>
                        prev.map((tab) =>
                            tab.id === existing.id
                                ? { ...tab, name: file.name }
                                : tab,
                        ),
                    );
                } else {
                    const newTab: EditorTab = {
                        id: nextEditorTabId.current++,
                        name: file.name,
                        type: "binary",
                        content: "// uploaded by user",
                    };
                    binaryTabId = newTab.id;
                    setEditorTabs((prev) => [...prev, newTab]);
                }

                setActiveEditorTabId(binaryTabId);
            } catch (err) {
                setCompileError(
                    err instanceof Error ? err.message : String(err),
                );
            } finally {
                setIsCompiling(false);
            }
        },
        [editorTabs, loadGraph],
    );

    const handleSelectFunction = useCallback(
        async (name: string) => {
            if (!graphSession || !name) return;
            setSelectedFunction(name);
            setSimulateResult(null);
            setSelectedBlockId(null);
            try {
                await loadGraph(graphSession, name);
            } catch (err) {
                setCompileError(
                    err instanceof Error ? err.message : String(err),
                );
            }
        },
        [graphSession, loadGraph],
    );

    const handleSimulate = useCallback(async () => {
        if (isSimulating || !selectedBlockId) return;

        const block = nodes.find((n) => n.id === selectedBlockId);
        if (!block) return;

        const instructions = block.instructions;

        setIsSimulating(true);
        setSimulateError(null);

        try {
            const { session } = await simulateStart({ cpu, instructions });
            const [general, instructionInfos, timelineEntries] =
                await Promise.all([
                    getGeneralResults(session),
                    getInstructionInfos(session),
                    getTimelineEntries(session),
                ]);

            setSimulateSession(session);
            setSimulateResult({
                general,
                instructions,
                instructionInfos,
                timelineEntries,
            });
            setSimulateMode(true);
        } catch (err) {
            setSimulateError(err instanceof Error ? err.message : String(err));
        } finally {
            setIsSimulating(false);
        }
    }, [nodes, selectedBlockId, isSimulating, cpu, uArch]);

    const handleBackFromSimulate = useCallback(() => {
        if (simulateSession) {
            endSimulate(simulateSession).catch(() => {});
        }
        setSimulateSession(null);
        setSimulateMode(false);
        setSimulateResult(null);
    }, [simulateSession]);

    const handleHover = useCallback(
        (region: InstructionRegion | null, x: number, y: number) => {
            if (hoverTimer.current) clearTimeout(hoverTimer.current);

            if (!region) {
                setHoveredRegion(null);
                setTooltipMetrics(null);
                return;
            }

            setHoveredRegion(region);
            setTooltipPos({ x, y });
            setTooltipMetrics(null);

            hoverTimer.current = setTimeout(async () => {
                try {
                    const metrics = await getInstructionBrief(
                        cpu,
                        region.instrText,
                    );
                    setTooltipMetrics(metrics);
                } catch {
                    setTooltipMetrics(null);
                }
            }, 120);
        },
        [cpu],
    );

    const handleInstructionClick = useCallback(
        (region: InstructionRegion) => {
            setModalRegion(region);
            setModalInfo(null);

            getInstructionFull(cpu, region.instrText)
                .then(setModalInfo)
                .catch(() => setModalInfo(null));
        },
        [cpu],
    );

    const getSimulateComponent = () => {
        if (!simulateResult) return null;

        switch (simulateTab) {
            case SimulateTab.General: {
                return <GeneralView {...simulateResult.general} />;
            }
            case SimulateTab.Instruction: {
                return (
                    <InstructionInfoView
                        instructionInfos={simulateResult.instructionInfos}
                        instructions={simulateResult.instructions}
                    />
                );
            }
            case SimulateTab.Timeline: {
                return (
                    <TimelineView
                        entries={simulateResult.timelineEntries}
                        instructions={simulateResult.instructions}
                    />
                );
            }
        }
    };

    return (
        <div className="flex flex-col h-screen overflow-hidden">
            <Header />
            <div className="flex flex-1 overflow-hidden">
                <div className="w-[42%] min-w-[380px] flex flex-col border-r border-base-300 bg-base-100">
                    <div className="flex items-center h-12 bg-base-200 border-b border-base-300 px-2">
                        {simulateMode ? (
                            <EditorTabsLocked
                                tabs={editorTabs}
                                activeId={activeEditorTabId}
                                onBack={handleBackFromSimulate}
                            />
                        ) : (
                            <EditorTabs
                                tabs={editorTabs}
                                activeId={activeEditorTabId}
                                onSelect={handleSelectEditorTab}
                                onClose={handleCloseEditorTab}
                                onAdd={handleAddEditorTab}
                                onUpload={handleUploadBinary}
                                compiler={compiler}
                                compilers={compilers}
                                onCompilerChange={setCompiler}
                                onCompile={handleCompile}
                                onOpenOptions={() => setCompileModalOpen(true)}
                                locked={activeEditorTab?.type === "binary"}
                            />
                        )}
                    </div>
                    <div className="flex-1 overflow-auto p-0">
                        {activeEditorTab && (
                            <Editor
                                key={activeEditorTab.id}
                                value={activeEditorTab.content}
                                onChange={handleEditorChange}
                                readOnly={activeEditorTab.type === "binary"}
                            />
                        )}
                    </div>
                </div>

                <div className="flex-1 flex flex-col bg-base-100">
                    {simulateMode ? (
                        <>
                            <SimulateToolbar
                                onTabSwitch={setSimulateTab}
                                defaultTab={simulateTab}
                            />
                            {simulateResult && (
                                <div className="flex-1 min-h-0 overflow-auto p-4 flex flex-col">
                                    {getSimulateComponent()}
                                </div>
                            )}
                        </>
                    ) : (
                        <>
                            {compileError && (
                                <div className="px-3 py-2 bg-red-100 text-red-800 text-sm border-b border-red-200">
                                    <div className="font-semibold">
                                        Compile error
                                    </div>
                                    <pre className="mt-1 whitespace-pre-wrap font-mono text-xs leading-relaxed">
                                        {compileError.trim()}
                                    </pre>
                                </div>
                            )}
                            <GraphToolbar
                                functions={functions}
                                selectedFunction={selectedFunction}
                                onFunctionChange={handleSelectFunction}
                                uArch={uArch}
                                uArchOptions={uArchOptions}
                                onUArchChange={setUArch}
                                onSimulate={handleSimulate}
                                graphLoaded={nodes.length > 0}
                                blockSelected={selectedBlockId !== null}
                                simulating={isSimulating}
                            />
                            <GraphView
                                nodes={nodes}
                                onHover={handleHover}
                                onClick={handleInstructionClick}
                                selectedNodeId={selectedBlockId}
                                onSelectNode={setSelectedBlockId}
                            />
                            {simulateError && (
                                <div className="px-3 py-2 bg-red-100 text-red-800 text-sm border-t border-red-200">
                                    <div className="font-semibold">
                                        Simulation error
                                    </div>
                                    <pre className="mt-1 whitespace-pre-wrap font-mono text-xs leading-relaxed">
                                        {simulateError.trim()}
                                    </pre>
                                </div>
                            )}
                        </>
                    )}
                </div>
            </div>

            {hoveredRegion && tooltipMetrics && (
                <InstructionTooltip
                    metrics={tooltipMetrics}
                    x={tooltipPos.x + 15}
                    y={tooltipPos.y - 20}
                />
            )}

            <InstructionModal
                region={modalRegion}
                info={modalInfo}
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

export default App;
