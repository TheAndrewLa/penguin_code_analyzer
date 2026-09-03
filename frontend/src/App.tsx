import { useState, useCallback, useRef } from "react";
import Header from "./components/Header";
import Toolbar from "./components/GraphToolbar";
import TabBar from "./components/EditorTabBar";
import Editor from "./components/Editor";
import GraphView from "./components/GraphView";
import InstructionTooltip from "./components/InstructionTooltip";
import InstructionModal from "./components/InstructionModal";
import EditOptionsModal from "./components/EditOptionsDialog";
import { EditorTab, InstructionRegion } from "./types";
import { nodes, defaultCppContent } from "./mock";

function App() {
    const [tabs, setTabs] = useState<EditorTab[]>([
        { id: 1, name: "main.cpp", type: "cpp", content: defaultCppContent },
    ]);
    const [activeTabId, setActiveTabId] = useState<number>(1);
    const nextId = useRef(2);

    const [hoveredRegion, setHoveredRegion] =
        useState<InstructionRegion | null>(null);
    const [tooltipPos, setTooltipPos] = useState({ x: 0, y: 0 });
    const [modalRegion, setModalRegion] = useState<InstructionRegion | null>(
        null,
    );

    const [scale, setScale] = useState(1);
    const [offset, setOffset] = useState({ x: 0, y: 0 });

    const compilers = ["gcc 13.2", "clang 22.0"];
    const [compiler, setCompiler] = useState(compilers[0]);
    const [compileFlags, setCompileFlags] = useState("");
    const [isCompileModalOpen, setCompileModalOpen] = useState(false);

    const handleAddTab = useCallback(
        (name: string, type: "c" | "cpp" | "asm") => {
            const newTab: EditorTab = {
                id: nextId.current++,
                name: name.concat(".", type),
                type: type,
                content: defaultCppContent,
            };
            setTabs((prev) => [...prev, newTab]);
            setActiveTabId(newTab.id);
        },
        [],
    );

    const handleCloseTab = useCallback(
        (id: number) => {
            setTabs((prev) => {
                if (prev.length <= 1) return prev;
                const idx = prev.findIndex((t) => t.id === id);
                if (idx === -1) return prev;
                const newTabs = [...prev];
                newTabs.splice(idx, 1);
                if (activeTabId === id) {
                    const newActive = newTabs[Math.max(0, idx - 1)];
                    setActiveTabId(newActive.id);
                }
                return newTabs;
            });
        },
        [activeTabId],
    );

    const handleSelectTab = useCallback((id: number) => {
        setActiveTabId(id);
    }, []);

    const handleEditorChange = useCallback(
        (value: string) => {
            setTabs((prev) =>
                prev.map((tab) =>
                    tab.id === activeTabId ? { ...tab, content: value } : tab,
                ),
            );
        },
        [activeTabId],
    );

    const activeTab = tabs.find((t) => t.id === activeTabId);

    const handleCompile = useCallback(() => {
        console.log(`Compiling with ${compiler}, flags: ${compileFlags}`);
        // Trigger compilation and update graph
    }, [compiler, compileFlags]);

    const handleSimulate = useCallback(() => {
        alert("Pipeline simulation (demo)");
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

    return (
        <div className="flex flex-col h-screen overflow-hidden">
            <Header />
            <div className="flex flex-1 overflow-hidden">
                <div className="w-[42%] min-w-[380px] flex flex-col border-r border-base-300 bg-base-100">
                    <div className="flex items-center h-12 bg-base-200 border-b border-base-300 px-2">
                        <TabBar
                            tabs={tabs}
                            activeId={activeTabId}
                            onSelect={handleSelectTab}
                            onClose={handleCloseTab}
                            onAdd={handleAddTab}
                            compiler={compiler}
                            compilers={compilers}
                            onCompilerChange={setCompiler}
                            onCompile={handleCompile}
                            onOpenOptions={() => setCompileModalOpen(true)}
                        />
                    </div>
                    <div className="flex-1 overflow-auto p-0">
                        {activeTab && (
                            <Editor
                                key={activeTab.id}
                                value={activeTab.content}
                                onChange={handleEditorChange}
                                language={
                                    activeTab.type === "asm" ? "asm" : "cpp"
                                }
                            />
                        )}
                    </div>
                </div>

                <div className="flex-1 flex flex-col bg-base-100">
                    <Toolbar onSimulate={handleSimulate} />
                    <GraphView
                        nodes={nodes}
                        onHover={handleHover}
                        onClick={setModalRegion}
                        scale={scale}
                        offset={offset}
                        setScale={setScale}
                        setOffset={setOffset}
                    />
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

export default App;
