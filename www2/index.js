(function () {
    "use strict";

    const INSTRUCTION_SET = [
        {
            uOps: 1,
            latency: 1,
            rThroughput: 0.25,
            mayLoad: false,
            mayStore: false,
            sideEffects: false,
            text: "movb $5, %ah",
        },
        {
            uOps: 1,
            latency: 1,
            rThroughput: 0.25,
            mayLoad: false,
            mayStore: false,
            sideEffects: false,
            text: "movb $5, %al",
        },
        {
            uOps: 1,
            latency: 1,
            rThroughput: 0.25,
            mayLoad: false,
            mayStore: false,
            sideEffects: false,
            text: "movl $6, %eax",
        },
        {
            uOps: 1,
            latency: 1,
            rThroughput: 0.25,
            mayLoad: false,
            mayStore: false,
            sideEffects: false,
            text: "movb $6, %al",
        },
        {
            uOps: 1,
            latency: 1,
            rThroughput: 0.25,
            mayLoad: false,
            mayStore: false,
            sideEffects: false,
            text: "movb $7, %ah",
        },
        {
            uOps: 1,
            latency: 1,
            rThroughput: 0.25,
            mayLoad: false,
            mayStore: false,
            sideEffects: false,
            text: "movl $7, %eax",
        },
        {
            uOps: 1,
            latency: 1,
            rThroughput: 0.25,
            mayLoad: false,
            mayStore: false,
            sideEffects: false,
            text: "movb $8, %ah",
        },
        {
            uOps: 1,
            latency: 1,
            rThroughput: 0.25,
            mayLoad: false,
            mayStore: false,
            sideEffects: false,
            text: "movb $8, %al",
        },
        {
            uOps: 3,
            latency: 7,
            rThroughput: 1.0,
            mayLoad: true,
            mayStore: true,
            sideEffects: false,
            text: "decl ecx",
        },
        {
            uOps: 1,
            latency: 1,
            rThroughput: 0.5,
            mayLoad: false,
            mayStore: false,
            sideEffects: false,
            text: "jne .loop",
        },
    ];

    const RESOURCE_NAMES = [
        "HWDivider",
        "HWFPDivider",
        "HWPort0",
        "HWPort1",
        "HWPort2",
        "HWPort3",
        "HWPort4",
        "HWPort5",
        "HWPort6",
        "HWPort7",
    ];
    const RESOURCE_PRESSURE_PER_ITER = [
        0, 0, 2.51, 2.5, 0.67, 0.67, 1.0, 2.5, 2.51, 0.67,
    ];

    const INSTRUCTION_PRESSURE = [
        [0, 0, 0, 0.01, 0, 0, 0, 0.5, 0.5, 0],
        [0, 0, 0.01, 0.5, 0, 0, 0, 0.5, 0, 0],
        [0, 0, 0.01, 0.5, 0, 0, 0, 0, 0.5, 0],
        [0, 0, 0.5, 0, 0, 0, 0, 0.5, 0.01, 0],
        [0, 0, 0, 0.5, 0, 0, 0, 0.01, 0.5, 0],
        [0, 0, 0.5, 0.01, 0, 0, 0, 0.5, 0, 0],
        [0, 0, 0.01, 0.5, 0, 0, 0, 0, 0.5, 0],
        [0, 0, 0.5, 0, 0, 0, 0, 0.5, 0.01, 0],
        [0, 0, 0, 0.5, 0.67, 0.67, 1.0, 0.01, 0.5, 0.67],
        [0, 0, 1.0, 0, 0, 0, 0, 0, 0.01, 0],
    ];

    // ---------- TIMELINE DATA (4 iterations, directly from the diagram) ----------
    const TIMELINE_ENTRIES = [
        // Iteration 0
        {
            iteration: 0,
            instructionIndex: 0,
            instructionText: "movb $5, %ah",
            dispatchCycles: [0],
            execCycles: [1],
            execEndCycles: [2],
            retireCycles: [3],
            waitQueueCycles: [],
            waitRetireCycles: [],
        },
        {
            iteration: 0,
            instructionIndex: 1,
            instructionText: "movb $5, %al",
            dispatchCycles: [0],
            execCycles: [1],
            execEndCycles: [2],
            retireCycles: [3],
            waitQueueCycles: [],
            waitRetireCycles: [],
        },
        {
            iteration: 0,
            instructionIndex: 2,
            instructionText: "movl $6, %eax",
            dispatchCycles: [0],
            execCycles: [1],
            execEndCycles: [2],
            retireCycles: [3],
            waitQueueCycles: [],
            waitRetireCycles: [],
        },
        {
            iteration: 0,
            instructionIndex: 3,
            instructionText: "movb $6, %al",
            dispatchCycles: [0],
            execCycles: [1],
            execEndCycles: [2],
            retireCycles: [3],
            waitQueueCycles: [],
            waitRetireCycles: [],
        },
        {
            iteration: 0,
            instructionIndex: 4,
            instructionText: "movb $7, %ah",
            dispatchCycles: [1],
            execCycles: [2],
            execEndCycles: [3],
            retireCycles: [4],
            waitQueueCycles: [],
            waitRetireCycles: [],
        },
        {
            iteration: 0,
            instructionIndex: 5,
            instructionText: "movl $7, %eax",
            dispatchCycles: [1],
            execCycles: [2],
            execEndCycles: [3],
            retireCycles: [4],
            waitQueueCycles: [],
            waitRetireCycles: [],
        },
        {
            iteration: 0,
            instructionIndex: 6,
            instructionText: "movb $8, %ah",
            dispatchCycles: [1],
            execCycles: [2],
            execEndCycles: [3],
            retireCycles: [4],
            waitQueueCycles: [],
            waitRetireCycles: [],
        },
        {
            iteration: 0,
            instructionIndex: 7,
            instructionText: "movb $8, %al",
            dispatchCycles: [1],
            execCycles: [2],
            execEndCycles: [3],
            retireCycles: [4],
            waitQueueCycles: [],
            waitRetireCycles: [],
        },
        {
            iteration: 0,
            instructionIndex: 8,
            instructionText: "decl ecx",
            dispatchCycles: [1],
            execCycles: [2, 3, 4, 5, 6, 7, 8],
            execEndCycles: [9],
            retireCycles: [10],
            waitQueueCycles: [],
            waitRetireCycles: [],
        },
        {
            iteration: 0,
            instructionIndex: 9,
            instructionText: "jne .loop",
            dispatchCycles: [1],
            execCycles: [9],
            execEndCycles: [10],
            retireCycles: [11],
            waitQueueCycles: [2, 3, 4, 5, 6, 7, 8],
            waitRetireCycles: [],
        },
        // Iteration 1
        {
            iteration: 1,
            instructionIndex: 0,
            instructionText: "movb $5, %ah",
            dispatchCycles: [2],
            execCycles: [3],
            execEndCycles: [4],
            retireCycles: [10],
            waitQueueCycles: [],
            waitRetireCycles: [5, 6, 7, 8, 9],
        },
        {
            iteration: 1,
            instructionIndex: 1,
            instructionText: "movb $5, %al",
            dispatchCycles: [2],
            execCycles: [3],
            execEndCycles: [4],
            retireCycles: [10],
            waitQueueCycles: [],
            waitRetireCycles: [5, 6, 7, 8, 9],
        },
        {
            iteration: 1,
            instructionIndex: 2,
            instructionText: "movl $6, %eax",
            dispatchCycles: [2],
            execCycles: [3],
            execEndCycles: [4],
            retireCycles: [10],
            waitQueueCycles: [],
            waitRetireCycles: [5, 6, 7, 8, 9],
        },
        {
            iteration: 1,
            instructionIndex: 3,
            instructionText: "movb $6, %al",
            dispatchCycles: [2],
            execCycles: [3],
            execEndCycles: [4],
            retireCycles: [10],
            waitQueueCycles: [],
            waitRetireCycles: [5, 6, 7, 8, 9],
        },
        {
            iteration: 1,
            instructionIndex: 4,
            instructionText: "movb $7, %ah",
            dispatchCycles: [3],
            execCycles: [4],
            execEndCycles: [5],
            retireCycles: [11],
            waitQueueCycles: [],
            waitRetireCycles: [6, 7, 8, 9, 10],
        },
        {
            iteration: 1,
            instructionIndex: 5,
            instructionText: "movl $7, %eax",
            dispatchCycles: [3],
            execCycles: [4],
            execEndCycles: [5],
            retireCycles: [11],
            waitQueueCycles: [],
            waitRetireCycles: [6, 7, 8, 9, 10],
        },
        {
            iteration: 1,
            instructionIndex: 6,
            instructionText: "movb $8, %ah",
            dispatchCycles: [3],
            execCycles: [4],
            execEndCycles: [5],
            retireCycles: [11],
            waitQueueCycles: [],
            waitRetireCycles: [6, 7, 8, 9, 10],
        },
        {
            iteration: 1,
            instructionIndex: 7,
            instructionText: "movb $8, %al",
            dispatchCycles: [3],
            execCycles: [4],
            execEndCycles: [5],
            retireCycles: [11],
            waitQueueCycles: [],
            waitRetireCycles: [6, 7, 8, 9, 10],
        },
        {
            iteration: 1,
            instructionIndex: 8,
            instructionText: "decl ecx",
            dispatchCycles: [4],
            execCycles: [5, 6, 7, 8, 9, 10, 11],
            execEndCycles: [12],
            retireCycles: [13],
            waitQueueCycles: [],
            waitRetireCycles: [],
        },
        {
            iteration: 1,
            instructionIndex: 9,
            instructionText: "jne .loop",
            dispatchCycles: [4],
            execCycles: [12],
            execEndCycles: [13],
            retireCycles: [14],
            waitQueueCycles: [5, 6, 7, 8, 9, 10, 11],
            waitRetireCycles: [],
        },
        // Iteration 2
        {
            iteration: 2,
            instructionIndex: 0,
            instructionText: "movb $5, %ah",
            dispatchCycles: [5],
            execCycles: [6],
            execEndCycles: [7],
            retireCycles: [13],
            waitQueueCycles: [],
            waitRetireCycles: [8, 9, 10, 11, 12],
        },
        {
            iteration: 2,
            instructionIndex: 1,
            instructionText: "movb $5, %al",
            dispatchCycles: [5],
            execCycles: [6],
            execEndCycles: [7],
            retireCycles: [13],
            waitQueueCycles: [],
            waitRetireCycles: [8, 9, 10, 11, 12],
        },
        {
            iteration: 2,
            instructionIndex: 2,
            instructionText: "movl $6, %eax",
            dispatchCycles: [5],
            execCycles: [6],
            execEndCycles: [7],
            retireCycles: [13],
            waitQueueCycles: [],
            waitRetireCycles: [8, 9, 10, 11, 12],
        },
        {
            iteration: 2,
            instructionIndex: 3,
            instructionText: "movb $6, %al",
            dispatchCycles: [5],
            execCycles: [6],
            execEndCycles: [7],
            retireCycles: [13],
            waitQueueCycles: [],
            waitRetireCycles: [8, 9, 10, 11, 12],
        },
        {
            iteration: 2,
            instructionIndex: 4,
            instructionText: "movb $7, %ah",
            dispatchCycles: [6],
            execCycles: [7],
            execEndCycles: [8],
            retireCycles: [14],
            waitQueueCycles: [],
            waitRetireCycles: [9, 10, 11, 12, 13],
        },
        {
            iteration: 2,
            instructionIndex: 5,
            instructionText: "movl $7, %eax",
            dispatchCycles: [6],
            execCycles: [7],
            execEndCycles: [8],
            retireCycles: [14],
            waitQueueCycles: [],
            waitRetireCycles: [9, 10, 11, 12, 13],
        },
        {
            iteration: 2,
            instructionIndex: 6,
            instructionText: "movb $8, %ah",
            dispatchCycles: [6],
            execCycles: [7],
            execEndCycles: [8],
            retireCycles: [14],
            waitQueueCycles: [],
            waitRetireCycles: [9, 10, 11, 12, 13],
        },
        {
            iteration: 2,
            instructionIndex: 7,
            instructionText: "movb $8, %al",
            dispatchCycles: [6],
            execCycles: [7],
            execEndCycles: [8],
            retireCycles: [14],
            waitQueueCycles: [],
            waitRetireCycles: [9, 10, 11, 12, 13],
        },
        {
            iteration: 2,
            instructionIndex: 8,
            instructionText: "decl ecx",
            dispatchCycles: [7],
            execCycles: [8, 9, 10, 11, 12, 13, 14],
            execEndCycles: [15],
            retireCycles: [16],
            waitQueueCycles: [],
            waitRetireCycles: [],
        },
        {
            iteration: 2,
            instructionIndex: 9,
            instructionText: "jne .loop",
            dispatchCycles: [7],
            execCycles: [15],
            execEndCycles: [16],
            retireCycles: [17],
            waitQueueCycles: [8, 9, 10, 11, 12, 13, 14],
            waitRetireCycles: [],
        },
        // Iteration 3
        {
            iteration: 3,
            instructionIndex: 0,
            instructionText: "movb $5, %ah",
            dispatchCycles: [8],
            execCycles: [9],
            execEndCycles: [10],
            retireCycles: [16],
            waitQueueCycles: [],
            waitRetireCycles: [11, 12, 13, 14, 15],
        },
        {
            iteration: 3,
            instructionIndex: 1,
            instructionText: "movb $5, %al",
            dispatchCycles: [8],
            execCycles: [9],
            execEndCycles: [10],
            retireCycles: [16],
            waitQueueCycles: [],
            waitRetireCycles: [11, 12, 13, 14, 15],
        },
        {
            iteration: 3,
            instructionIndex: 2,
            instructionText: "movl $6, %eax",
            dispatchCycles: [8],
            execCycles: [9],
            execEndCycles: [10],
            retireCycles: [16],
            waitQueueCycles: [],
            waitRetireCycles: [11, 12, 13, 14, 15],
        },
        {
            iteration: 3,
            instructionIndex: 3,
            instructionText: "movb $6, %al",
            dispatchCycles: [8],
            execCycles: [9],
            execEndCycles: [10],
            retireCycles: [16],
            waitQueueCycles: [],
            waitRetireCycles: [11, 12, 13, 14, 15],
        },
        {
            iteration: 3,
            instructionIndex: 4,
            instructionText: "movb $7, %ah",
            dispatchCycles: [9],
            execCycles: [10],
            execEndCycles: [11],
            retireCycles: [17],
            waitQueueCycles: [],
            waitRetireCycles: [12, 13, 14, 15, 16],
        },
        {
            iteration: 3,
            instructionIndex: 5,
            instructionText: "movl $7, %eax",
            dispatchCycles: [9],
            execCycles: [10],
            execEndCycles: [11],
            retireCycles: [17],
            waitQueueCycles: [],
            waitRetireCycles: [12, 13, 14, 15, 16],
        },
        {
            iteration: 3,
            instructionIndex: 6,
            instructionText: "movb $8, %ah",
            dispatchCycles: [9],
            execCycles: [10],
            execEndCycles: [11],
            retireCycles: [17],
            waitQueueCycles: [],
            waitRetireCycles: [12, 13, 14, 15, 16],
        },
        {
            iteration: 3,
            instructionIndex: 7,
            instructionText: "movb $8, %al",
            dispatchCycles: [9],
            execCycles: [10],
            execEndCycles: [11],
            retireCycles: [17],
            waitQueueCycles: [],
            waitRetireCycles: [12, 13, 14, 15, 16],
        },
        {
            iteration: 3,
            instructionIndex: 8,
            instructionText: "decl ecx",
            dispatchCycles: [10],
            execCycles: [11, 12, 13, 14, 15, 16, 17],
            execEndCycles: [18],
            retireCycles: [19],
            waitQueueCycles: [],
            waitRetireCycles: [],
        },
        {
            iteration: 3,
            instructionIndex: 9,
            instructionText: "jne .loop",
            dispatchCycles: [10],
            execCycles: [18],
            execEndCycles: [19],
            retireCycles: [20],
            waitQueueCycles: [11, 12, 13, 14, 15, 16, 17],
            waitRetireCycles: [],
        },
    ];

    // ---------- GLOBALS ----------
    let editor = null;
    let activeTab = "general";
    let canvasScale = 1.0;
    let currentPanX = 0,
        currentPanY = 0;
    let isDragging = false;
    let dragStartX = 0,
        dragStartY = 0;
    let showLegend = false;
    const canvasElem = document.getElementById("analyzerCanvas");
    const graphContainer = document.getElementById("graphContainer");
    let ctx = null;
    let canvasWidth = 0,
        canvasHeight = 0;

    // ---------- CodeMirror Setup (read-only) ----------
    const editorWrapper = document.getElementById("editorWrapper");
    const textarea = document.createElement("textarea");
    textarea.id = "asm-editor";
    editorWrapper.appendChild(textarea);
    editor = CodeMirror.fromTextArea(textarea, {
        lineNumbers: true,
        mode: "gas",
        theme: "eclipse",
        indentUnit: 4,
        tabSize: 4,
        viewportMargin: Infinity,
        matchBrackets: true,
        readOnly: true,
        cursorBlinkRate: -1,
    });
    const sampleAsm = `movb $5, %ah
movb $5, %al
movl $6, %eax
movb $6, %al
movb $7, %ah
movl $7, %eax
movb $8, %ah
movb $8, %al
decl ecx
jne .loop`;
    editor.setValue(sampleAsm);

    // ---------- Canvas helpers ----------
    function resizeCanvas() {
        if (!graphContainer) return;
        const rect = graphContainer.getBoundingClientRect();
        canvasWidth = rect.width;
        canvasHeight = rect.height;
        canvasElem.width = canvasWidth;
        canvasElem.height = canvasHeight;
        canvasElem.style.width = `${canvasWidth}px`;
        canvasElem.style.height = `${canvasHeight}px`;
        ctx = canvasElem.getContext("2d");
        ctx.font = `14px "IBM Plex Mono", monospace`;
        ctx.textRendering = "geometricPrecision";
        redrawCanvas();
    }

    function updateZoomUI() {
        document.getElementById("zoomLevel").innerText =
            Math.round(canvasScale * 100) + "%";
    }

    function setZoom(scale) {
        canvasScale = Math.min(3.0, Math.max(0.3, scale));
        updateZoomUI();
        redrawCanvas();
    }

    function zoomIn() {
        setZoom(canvasScale + 0.1);
    }
    function zoomOut() {
        setZoom(canvasScale - 0.1);
    }
    function resetZoom() {
        setZoom(1.0);
        currentPanX = 0;
        currentPanY = 0;
        redrawCanvas();
    }

    // Legend items per tab
    function getLegendItems() {
        if (activeTab === "info") {
            return [
                "ML = MayLoad",
                "MS = MayStore",
                "SideFx = Side Effects",
                "Lat = Latency",
                "uOps = micro-ops",
            ];
        } else if (activeTab === "resource") {
            return RESOURCE_NAMES.map((name) => name.substring(0, 8));
        } else if (activeTab === "timeline") {
            return [
                "D = Dispatch",
                "e = Execute",
                "E = Exec End",
                "R = Retire",
                "= = Waiting for data",
                "- = Waiting to execute",
                ". = Idle",
            ];
        }
        return [];
    }

    function drawLegend() {
        if (!showLegend) return;
        const items = getLegendItems();
        if (items.length === 0) return;
        const padding = 10;
        const lineHeight = 18;
        const boxWidth = 180;
        const boxHeight = items.length * lineHeight + 2 * padding;
        const margin = Math.min(canvasWidth * 0.05, 30);
        const x = canvasWidth - boxWidth - margin;
        const y = margin;
        ctx.save();
        ctx.shadowBlur = 0;
        ctx.fillStyle = "rgba(255,255,240,0.96)";
        ctx.fillRect(x, y, boxWidth, boxHeight);
        ctx.strokeStyle = "#aaa";
        ctx.lineWidth = 1;
        ctx.strokeRect(x, y, boxWidth, boxHeight);
        ctx.fillStyle = "#1e1e1e";
        ctx.font = `bold 12px "Inter"`;
        ctx.fillText("Legend", x + padding, y + 18);
        ctx.font = `11px "IBM Plex Mono"`;
        for (let i = 0; i < items.length; i++) {
            ctx.fillText(
                items[i],
                x + padding,
                y + padding + 20 + i * lineHeight,
            );
        }
        ctx.restore();
    }

    function redrawCanvas() {
        if (!ctx || canvasWidth === 0 || canvasHeight === 0) return;
        ctx.clearRect(0, 0, canvasWidth, canvasHeight);
        ctx.save();
        ctx.translate(currentPanX, currentPanY);
        ctx.scale(canvasScale, canvasScale);
        ctx.textRendering = "geometricPrecision";
        if (activeTab === "general") drawGeneralResults();
        else if (activeTab === "info") drawInstructionInfo();
        else if (activeTab === "resource") drawResourceUsage();
        else if (activeTab === "timeline") drawTimelineView();
        ctx.restore();
        drawLegend();
    }

    // ------------------- DRAWING FUNCTIONS -------------------
    function drawGeneralResults() {
        const metrics = [
            { label: "Iterations:", value: "200" },
            { label: "Instructions:", value: "2000" },
            { label: "Total Cycles:", value: "610" },
            { label: "Total uOps:", value: "2400" },
            { label: "Dispatch Width:", value: "4" },
            { label: "uOps Per Cycle:", value: "3.93" },
            { label: "IPC:", value: "3.28" },
            { label: "Block RThroughput:", value: "3.0" },
        ];
        const startX = 40;
        let startY = 40;
        ctx.font = `18px "IBM Plex Mono"`;
        ctx.fillStyle = "#1e1e1e";
        metrics.forEach((m, idx) => {
            ctx.fillText(`${m.label}`, startX, startY + idx * 38);
            ctx.font = `18px "Inter"`;
            ctx.fillStyle = "#2c3e50";
            ctx.fillText(`${m.value}`, startX + 250, startY + idx * 38);
            ctx.font = `18px "IBM Plex Mono"`;
            ctx.fillStyle = "#1e1e1e";
        });
    }

    function drawInstructionInfo() {
        const startX = 40;
        const startY = 40;
        const colX = [0, 55, 110, 180, 240, 310, 380];
        const headers = [
            "#uOps",
            "Lat",
            "RThr",
            "ML",
            "MS",
            "SideFx",
            "Instructions",
        ];
        ctx.font = `14px "Inter"`;
        ctx.fillStyle = "#1e1e1e";
        for (let i = 0; i < headers.length; i++) {
            ctx.fillText(
                headers[i],
                startX + (i < 6 ? colX[i] : colX[6]),
                startY + 6,
            );
        }
        ctx.strokeStyle = "#c0c0c0";
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(startX, startY + 12);
        ctx.lineTo(startX + 620, startY + 12);
        ctx.stroke();

        let rowY = startY + 32;
        for (let idx = 0; idx < INSTRUCTION_SET.length; idx++) {
            const ins = INSTRUCTION_SET[idx];
            ctx.font = `13px "IBM Plex Mono"`;
            ctx.fillStyle = "#2c3e50";
            ctx.fillText(ins.uOps.toString(), startX + colX[0], rowY);
            ctx.fillText(ins.latency.toString(), startX + colX[1], rowY);
            ctx.fillText(ins.rThroughput.toFixed(2), startX + colX[2], rowY);
            ctx.fillText(ins.mayLoad ? "✓" : "-", startX + colX[3], rowY);
            ctx.fillText(ins.mayStore ? "✓" : "-", startX + colX[4], rowY);
            ctx.fillText(ins.sideEffects ? "U" : "-", startX + colX[5], rowY);
            ctx.fillText(ins.text, startX + colX[6], rowY);
            rowY += 24;
        }
    }

    function drawResourceUsage() {
        let yOff = 35;
        ctx.font = `bold 14px "Inter"`;
        ctx.fillStyle = "#0e639c";
        ctx.fillText("RESOURCE PRESSURE PER ITERATION", 20, yOff);
        yOff += 28;
        const barStartX = 25;
        const barWidth = 45;
        const maxPressure = 3.5;
        for (let i = 0; i < RESOURCE_NAMES.length; i++) {
            let pressure = RESOURCE_PRESSURE_PER_ITER[i];
            let x = barStartX + (i % 5) * 115;
            let rowIdx = Math.floor(i / 5);
            let barY = yOff + rowIdx * 55;
            ctx.fillStyle = "#555";
            ctx.font = `11px "Inter"`;
            ctx.fillText(RESOURCE_NAMES[i].substring(0, 7), x, barY - 4);
            let barHeight = (pressure / maxPressure) * 40;
            ctx.fillStyle = "#fbb613";
            ctx.fillRect(x, barY, barWidth - 2, barHeight);
            ctx.fillStyle = "#1e1e1e";
            ctx.fillText(pressure.toFixed(2), x + 8, barY + barHeight + 14);
        }
        yOff += 150;
        ctx.font = `bold 14px "Inter"`;
        ctx.fillStyle = "#0e639c";
        ctx.fillText("RESOURCE PRESSURE BY INSTRUCTION", 20, yOff);
        yOff += 22;
        ctx.font = `11px "IBM Plex Mono"`;
        ctx.fillStyle = "#3a3a3a";
        const headX = [25, 65, 105, 145, 185, 225, 265, 305, 345, 385, 430];
        for (let p = 0; p <= 9; p++) {
            ctx.fillText(`P${p}`, headX[p] + 5, yOff);
        }
        ctx.fillText("Instruction", headX[10] + 5, yOff);
        yOff += 20;
        for (let i = 0; i < INSTRUCTION_PRESSURE.length; i++) {
            const press = INSTRUCTION_PRESSURE[i];
            for (let p = 0; p < press.length; p++) {
                ctx.fillStyle = "#1e1e1e";
                ctx.fillText(
                    press[p].toFixed(2).replace(/^0\./, "."),
                    headX[p] + 5,
                    yOff + 2,
                );
            }
            ctx.fillStyle = "#555";
            ctx.fillText(
                INSTRUCTION_SET[i].text.substring(0, 26),
                headX[10] + 5,
                yOff + 2,
            );
            yOff += 22;
        }
    }

    function drawTimelineView() {
        const startX = 60;
        let startY = 40;

        let maxCycles = 0;
        for (const entry of TIMELINE_ENTRIES) {
            const lastCycle = Math.max(
                ...entry.dispatchCycles,
                ...entry.execCycles,
                ...entry.execEndCycles,
                ...entry.retireCycles,
                ...entry.waitQueueCycles,
                ...entry.waitRetireCycles,
                -1,
            );
            if (lastCycle + 1 > maxCycles) maxCycles = lastCycle + 1;
        }

        maxCycles = Math.min(maxCycles, 100);

        const cellW = 18;
        const cellH = 24;
        let currentY = startY;

        for (let idx = 0; idx < TIMELINE_ENTRIES.length; idx++) {
            const entry = TIMELINE_ENTRIES[idx];
            // if (currentY + cellH > canvasHeight / canvasScale + 200) break;

            ctx.fillStyle = "#7f8c8d";
            ctx.font = `11px monospace`;
            ctx.fillText(
                `${entry.iteration},${entry.instructionIndex}`,
                startX - 45,
                currentY + 15,
            );

            for (let cycle = 0; cycle < maxCycles; cycle++) {
                const xPos = startX + cycle * cellW;
                const yPos = currentY;

                let state = ".";
                if (entry.dispatchCycles.includes(cycle)) state = "D";
                else if (entry.execCycles.includes(cycle)) state = "e";
                else if (entry.execEndCycles.includes(cycle)) state = "E";
                else if (entry.retireCycles.includes(cycle)) state = "R";
                else if (entry.waitQueueCycles.includes(cycle)) state = "=";
                else if (entry.waitRetireCycles.includes(cycle)) state = "-";

                switch (state) {
                    case "D":
                        ctx.fillStyle = "#2c7cb0";
                        break;
                    case "e":
                        ctx.fillStyle = "#e67e22";
                        break;
                    case "E":
                        ctx.fillStyle = "#f1c40f";
                        break;
                    case "R":
                        ctx.fillStyle = "#27ae60";
                        break;
                    case "=":
                        ctx.fillStyle = "#b8860b";
                        break;
                    case "-":
                        ctx.fillStyle = "#95a5a6";
                        break;
                    default:
                        ctx.fillStyle = "#ecf0f1";
                        break;
                }
                ctx.fillRect(xPos, yPos, cellW - 1, cellH - 2);
                ctx.fillStyle = "#1e1e1e";
                ctx.font = `10px monospace`;
                if (state !== ".") {
                    ctx.fillText(state, xPos + 5, yPos + 16);
                }
            }

            ctx.fillStyle = "#2c3e50";
            ctx.font = `11px "IBM Plex Mono"`;
            ctx.fillText(
                entry.instructionText.substring(0, 30),
                startX + maxCycles * cellW + 12,
                currentY + 16,
            );
            currentY += cellH;
        }
    }

    function switchTab(tabId) {
        activeTab = tabId;
        document.querySelectorAll(".tab").forEach((tab) => {
            const tabVal = tab.getAttribute("data-tab");
            if (tabVal === tabId) tab.classList.add("active");
            else tab.classList.remove("active");
        });
        resetZoom();
        redrawCanvas();
    }

    function bindEvents() {
        document.getElementById("zoomInBtn").addEventListener("click", zoomIn);
        document
            .getElementById("zoomOutBtn")
            .addEventListener("click", zoomOut);
        document
            .getElementById("zoomLevel")
            .addEventListener("click", resetZoom);
        document
            .getElementById("backToEditorBtn")
            .addEventListener("click", () => editor.focus());

        const legendBtn = document.getElementById("legendToggleBtn");

        legendBtn.addEventListener("click", () => {
            if (activeTab !== "general") {
                showLegend = !showLegend;
                legendBtn.classList.toggle("active", showLegend);
                redrawCanvas();
            } else {
                if (showLegend) {
                    showLegend = false;
                    legendBtn.classList.remove("active");
                    redrawCanvas();
                }
            }
        });

        document.querySelectorAll(".tab").forEach((tab) => {
            tab.addEventListener("click", () => {
                const tabId = tab.getAttribute("data-tab");
                if (tabId) {
                    switchTab(tabId);
                    if (activeTab === "general") {
                        showLegend = false;
                        document
                            .getElementById("legendToggleBtn")
                            .classList.remove("active");
                    } else if (showLegend) {
                        redrawCanvas();
                    }
                }
            });
        });

        graphContainer.addEventListener("mousedown", (e) => {
            isDragging = true;
            dragStartX = e.clientX - currentPanX;
            dragStartY = e.clientY - currentPanY;
            graphContainer.style.cursor = "grabbing";
            e.preventDefault();
        });

        window.addEventListener("mousemove", (e) => {
            if (!isDragging) return;
            currentPanX = e.clientX - dragStartX;
            currentPanY = e.clientY - dragStartY;
            currentPanX = Math.min(800, Math.max(-800, currentPanX));
            currentPanY = Math.min(800, Math.max(-800, currentPanY));
            redrawCanvas();
        });

        window.addEventListener("mouseup", () => {
            isDragging = false;
            graphContainer.style.cursor = "grab";
        });

        graphContainer.addEventListener("wheel", (e) => {
            e.preventDefault();
            const delta = e.deltaY > 0 ? 0.9 : 1.1;
            const newScale = Math.min(Math.max(canvasScale * delta, 0.3), 3.0);
            setZoom(newScale);
        });

        window.addEventListener("resize", () => {
            resizeCanvas();
        });

        document
            .getElementById("saveBtn")
            .addEventListener("click", () =>
                alert(
                    "Save project (demo): assembly + analysis state would be saved.",
                ),
            );
        document
            .getElementById("shareBtn")
            .addEventListener("click", () =>
                alert("Share link (demo): unique URL for this analysis."),
            );
    }

    function init() {
        bindEvents();
        resizeCanvas();
        switchTab("general");
        updateZoomUI();
        editor.setSize(null, "100%");
        editor.refresh();
    }

    init();
})();
