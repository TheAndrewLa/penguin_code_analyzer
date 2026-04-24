$(function () {
  "use strict";

  const tabBar = $("#tabBar");
  const editorWrapper = $("#editorWrapper");
  const sourceMenu = $("#sourceMenu");
  const fileInput = $("#fileInput");

  let tabs = [];
  let activeTabId = null;
  let nextTabId = 1;

  function updateTabsCloseButtons() {
    tabs.forEach((tab) => {
      const close = tab.tab.find(".tab-close");

      if (tabs.length === 1) {
        close.hide();
      } else {
        close.show();
      }
    });
  }

  function createTab(type, customName, initialContent) {
    const id = nextTabId++;
    let name = customName;
    if (!name) {
      if (type === "c") name = "main.c";
      else if (type === "cpp") name = "main.cpp";
      else if (type === "asm") name = "main.s";
      else name = "source";
    }

    const addButton = $("#addTabBtn");

    const tab = $("<div>", {
      class: "tab",
      "data-id": id,
    });

    const tabName = $("<span>").text(name).css("margin-right", "8px");

    const closeButton = $("<span>", {
      class: "tab-close",
      html: "&times;",
      title: "Close tab",
    }).css({
      cursor: "pointer",
      fontWeight: "bold",
      fontSize: "16px",
      padding: "0 4px",
    });

    tab.append(tabName, closeButton);
    tab.insertBefore(addButton);

    const textarea = $("<textarea>", {
      id: `editor-${id}`,
      style: "display:none;",
    }).appendTo(editorWrapper);

    let mode = "text/x-c++src";

    if (type === "c") mode = "text/x-csrc";
    else if (type === "asm") mode = "gas";

    const editor = CodeMirror.fromTextArea(textarea[0], {
      lineNumbers: true,
      mode: mode,
      theme: "eclipse",
      indentUnit: 4,
      tabSize: 4,
      viewportMargin: Infinity,
      gutters: ["CodeMirror-linenumbers"],
      matchBrackets: true,
    });

    if (initialContent) {
      editor.setValue(initialContent);
    } else {
      if (type === "asm") {
        editor.setValue(
          `.globl fib\nfib:\n    test edi, edi\n    jle .L4\n    mov eax,1\n    mov edx,1\n.L3:\n    sub edi,1\n    lea ecx,[rdx+rax]\n    mov edx,eax\n    mov eax,ecx\n    cmp edi,-1\n    jne .L3\n.L4:\n    ret`,
        );
      } else {
        editor.setValue(
          `int fib(int n) {\n    int prev = 1, current = 1;\n    for (; n >= 0; --n) {\n        int tmp = prev + current;\n        prev = current;\n        current = tmp;\n    }\n    return current;\n}`,
        );
      }
    }

    editor.setSize("100%", "100%");

    const tabInfo = {
      id,
      type,
      name,
      editor,
      tab,
      textarea,
    };

    tabs.push(tabInfo);

    tab.on("click", function (e) {
      if ($(e.target).is(".tab-close")) return;
      setActiveTab(id);
    });

    closeButton.on("click", function (e) {
      e.stopPropagation();
      closeTab(id);
    });

    updateTabsCloseButtons();

    return tabInfo;
  }

  function setActiveTab(id) {
    if (activeTabId === id) return;

    tabs.forEach((tab) => {
      tab.editor.getWrapperElement().style.display = "none";
      tab.tab.removeClass("active");
    });

    const activeTab = tabs.find((t) => t.id === id);

    if (activeTab) {
      activeTab.editor.getWrapperElement().style.display = "block";
      activeTab.tab.addClass("active");
      setTimeout(() => {
        activeTab.editor.refresh();
        activeTab.editor.focus();
      }, 20);
      activeTabId = id;
    }
  }

  function closeTab(id) {
    const index = tabs.findIndex((t) => t.id === id);
    if (index === -1) return;

    if (tabs.length <= 1) {
      alert("Cannot close the last tab.");
      return;
    }

    const tab = tabs[index];
    tab.tab.remove();
    tab.editor.getWrapperElement().remove();
    tabs.splice(index, 1);

    if (activeTabId === id) {
      const newActive = tabs[Math.max(0, index - 1)];
      setActiveTab(newActive.id);
    }

    updateTabsCloseButtons();
  }

  const $addTabBtn = $("<div>", {
    class: "tab add-tab",
    id: "addTabBtn",
    text: "+",
  }).appendTo(tabBar);

  const initialTab = createTab("cpp", "main.cpp");

  initialTab.tab.addClass("active");
  initialTab.editor.getWrapperElement().style.display = "block";
  activeTabId = initialTab.id;

  setTimeout(() => {
    initialTab.editor.refresh();
    initialTab.editor.focus();
  }, 50);

  $addTabBtn.on("click", function (e) {
    e.stopPropagation();
    sourceMenu.toggle();
  });

  $(document).on("click", function () {
    sourceMenu.hide();
  });

  sourceMenu.on("click", ".menu-item", function (e) {
    const type = $(this).data("type");
    sourceMenu.hide();

    if (type === "binary") {
      fileInput.trigger("click");
      fileInput.off("change").on("change", function (e) {
        const file = e.target.files[0];
        if (file) {
          const tab = createTab(
            "binary",
            file.name,
            `; Binary file: ${file.name}\n; Content not shown`,
          );
          setActiveTab(tab.id);
        }
        fileInput.val("");
      });
    } else {
      const tab = createTab(type);
      setActiveTab(tab.id);
    }
  });

  const canvas = $("#cfgCanvas");
  const canvasObject = canvas[0];
  const ctx = canvasObject.getContext("2d");
  const container = $("#graphContainer");

  const nodes = [
    {
      id: "L1",
      label: "L1",
      instructions: ["test edi, edi", "js L4"],
      edgesOut: [
        { to: "L2", fallthrough: true, conditional: true, taken: false },
        { to: "L4", conditional: true, taken: true },
      ],
    },
    {
      id: "L2",
      label: "L2",
      instructions: ["mov eax, 1", "mov edx, 1", "jmp L3"],
      edgesOut: [{ to: "L3" }],
    },
    {
      id: "L5",
      label: "L5",
      instructions: ["mov eax, ecx"],
      edgesOut: [{ to: "L3", fallthrough: true }],
    },
    {
      id: "L3",
      label: "L3",
      instructions: [
        "sub edi, 1",
        "lea ecx, [rdx+rax]",
        "mov edx, eax",
        "cmp edi, -1",
        "jne L3",
      ],
      edgesOut: [
        { to: "L5", conditional: true, taken: true },
        { to: "L6", conditional: true, taken: false },
      ],
    },
    {
      id: "L4",
      label: "L4",
      instructions: ["mov eax, 1", "ret"],
      edgesOut: [],
    },
    {
      id: "L6",
      label: "L6",
      instructions: ["mov eax, ecx", "ret"],
      edgesOut: [],
    },
  ];

  let instructionHitRegions = [];

  let scale = 1.0;

  let offsetX = 0;
  let offsetY = 0;

  let isPanning = false;
  let startPanX, startPanY;

  const $zoomLevel = $("#zoomLevel");

  function updateZoomDisplay() {
    $zoomLevel.text(Math.round(scale * 100) + "%");
  }

  function drawGraph() {
    const containerRect = container[0].getBoundingClientRect();
    canvasObject.width = containerRect.width;
    canvasObject.height = containerRect.height;
    canvas.css({ width: containerRect.width, height: containerRect.height });

    ctx.clearRect(0, 0, canvasObject.width, canvasObject.height);

    ctx.save();
    ctx.translate(offsetX, offsetY);
    ctx.scale(scale, scale);

    instructionHitRegions = [];

    let nodesCoordinates = new Map();

    let currentX = 50;
    let currentY = 20;

    nodes.forEach((node) => {
      let width = 240;
      const height = 40 + node.instructions.length * 24;

      node.instructions.forEach((instr, idx) => {
        ctx.font = '15px "IBM Plex Mono"';
        const textWidth = ctx.measureText(instr).width;

        if (textWidth >= width - 10) {
          width = textWidth - 10;
        }
      });

      ctx.fillStyle = "#ffffff";
      ctx.strokeStyle = "#000000";
      ctx.lineWidth = 1;
      ctx.lineJoin = "round";

      ctx.fillRect(currentX, currentY, width, height);
      ctx.strokeRect(currentX, currentY, width, height);

      ctx.fillStyle = "#1e1e1e";
      ctx.font = 'bold 18px "Inter';

      ctx.fillText(node.label, currentX + 10, currentY + 22);

      ctx.font = '16px "IBM Plex Mono"';
      ctx.fillStyle = "#333";

      let yOffset = currentY + 48;

      node.instructions.forEach((instr, idx) => {
        const textWidth = ctx.measureText(instr).width;

        instructionHitRegions.push({
          nodeId: node.id,
          instrIndex: idx,
          instrText: instr,
          rect: { x: currentX + 10, y: yOffset - 16, w: textWidth + 8, h: 20 },
        });

        ctx.fillText(instr, currentX + 12, yOffset);
        yOffset += 22;
      });

      nodesCoordinates.set(node.id, {
        x: currentX,
        y: currentY,
        width: width,
        height: height,
      });

      currentY += height;
      currentY += 50;
    });

    let usedSpace = 15;

    nodes.forEach((node) => {
      const startCoords = nodesCoordinates.get(node.id);

      node.edgesOut.forEach((edge, idx) => {
        const endCoords = nodesCoordinates.get(edge.to);

        const width = startCoords.width / (node.edgesOut.length + 1);

        const startX = startCoords.x + width * (idx + 1);
        const startY = startCoords.y + startCoords.height;

        const endX = startX;
        const endY = endCoords.y;

        const isFall = edge.fallthrough || false;
        const isConditional = edge.conditional || false;
        const isTaken = edge.taken || false;

        const conditionColor = isTaken ? "#2e7d32" : "#992017";
        const lineColor = isConditional ? conditionColor : "#000000";

        ctx.beginPath();
        ctx.strokeStyle = lineColor;
        ctx.lineWidth = 1;

        ctx.moveTo(startX, startY);

        if (isFall) {
          ctx.lineTo(startX, endY);
        } else {
          ctx.lineTo(startX, startY + usedSpace / 2);
          ctx.lineTo(
            startCoords.x + startCoords.width + usedSpace + 10,
            startCoords.y + startCoords.height + usedSpace / 2,
          );
          ctx.lineTo(
            endCoords.x + endCoords.width + usedSpace + 10,
            endY - usedSpace / 2,
          );
          ctx.lineTo(endX, endY - usedSpace / 2);
          ctx.lineTo(endX, endY);
          usedSpace += 10;
        }

        ctx.stroke();
      });
    });

    ctx.restore();
    updateZoomDisplay();
  }

  container.on("wheel", function (e) {
    e.preventDefault();
    const delta = e.originalEvent.deltaY > 0 ? 0.9 : 1.1;
    const newScale = Math.min(Math.max(scale * delta, 0.3), 3.0);

    const rect = canvasObject.getBoundingClientRect();
    const mouseX = e.clientX - rect.left;
    const mouseY = e.clientY - rect.top;

    const worldX = (mouseX - offsetX) / scale;
    const worldY = (mouseY - offsetY) / scale;

    scale = newScale;
    offsetX = mouseX - worldX * scale;
    offsetY = mouseY - worldY * scale;

    drawGraph();
  });

  container.on("mousedown", function (e) {
    isPanning = true;
    startPanX = e.clientX - offsetX;
    startPanY = e.clientY - offsetY;
    container.css("cursor", "grabbing");
  });

  $(window).on("mousemove", function (e) {
    if (!isPanning) return;
    offsetX = e.clientX - startPanX;
    offsetY = e.clientY - startPanY;
    drawGraph();
  });

  $(window).on("mouseup", function () {
    isPanning = false;
    container.css("cursor", "grab");
  });

  $("#zoomInBtn").on("click", function () {
    scale = Math.min(scale * 1.2, 3.0);
    drawGraph();
  });

  $("#zoomOutBtn").on("click", function () {
    scale = Math.max(scale / 1.2, 0.3);
    drawGraph();
  });

  $zoomLevel.on("click", function () {
    scale = 1.0;
    offsetX = 0;
    offsetY = 0;
    drawGraph();
  });

  const tooltip = $("#insnTooltip");
  const modalOverlay = $("#modalOverlay");
  const modalPre = $("#modalPre");
  const closeModalButton = $("#closeModalBtn");

  function showTooltip(text, x, y) {
    tooltip.css({ display: "block", left: x + 15, top: y - 30 }).html(text);
  }

  function hideTooltip() {
    tooltip.hide();
  }

  function getMetrics(insn) {
    const lower = insn.toLowerCase();
    if (lower.includes("test") || lower.includes("cmp"))
      return { lat: 1, tp: 0.25, uops: 1 };
    if (lower.includes("mov")) return { lat: 1, tp: 0.25, uops: 1 };
    if (lower.includes("lea")) return { lat: 1, tp: 0.5, uops: 1 };
    if (lower.includes("add") || lower.includes("sub"))
      return { lat: 1, tp: 0.25, uops: 1 };
    if (lower.includes("j")) return { lat: 1, tp: 0.5, uops: 1 };
    return { lat: 1, tp: 0.33, uops: 1 };
  }

  function getIntelSnippet(insn) {
    return `${insn}\n Intel® 64 and IA-32 Architectures Software Developer’s Manual\n Vol. 2A 3-XXX\n Opcode: ...\n Description: ...\n Flags affected: OF,SF,ZF,AF,PF,CF`;
  }

  canvas.on("mousemove", function (e) {
    const rect = canvasObject.getBoundingClientRect();
    const mouseX = e.clientX - rect.left;
    const mouseY = e.clientY - rect.top;

    const worldX = (mouseX - offsetX) / scale;
    const worldY = (mouseY - offsetY) / scale;

    let hit = null;
    for (let region of instructionHitRegions) {
      const r = region.rect;
      if (
        worldX >= r.x &&
        worldX <= r.x + r.w &&
        worldY >= r.y &&
        worldY <= r.y + r.h
      ) {
        hit = region;
        break;
      }
    }
    if (hit) {
      const m = getMetrics(hit.instrText);
      const content = `<strong>${hit.instrText}</strong><br>Latency: ${m.lat}<br>Throughput: ${m.tp}<br>uOps: ${m.uops}<br><span style="color:#666; font-size:11px;">Ctrl+Click — Intel manual</span>`;
      showTooltip(content, e.clientX, e.clientY);
    } else {
      hideTooltip();
    }
  });

  canvas.on("mouseleave", hideTooltip);

  canvas.on("click", function (e) {
    if (!e.ctrlKey) return;
    const rect = canvasObject.getBoundingClientRect();
    const mouseX = e.clientX - rect.left;
    const mouseY = e.clientY - rect.top;
    const worldX = (mouseX - offsetX) / scale;
    const worldY = (mouseY - offsetY) / scale;

    for (let region of instructionHitRegions) {
      const r = region.rect;
      if (
        worldX >= r.x &&
        worldX <= r.x + r.w &&
        worldY >= r.y &&
        worldY <= r.y + r.h
      ) {
        modalPre.text(getIntelSnippet(region.instrText));
        modalOverlay.css("visibility", "visible");
        e.preventDefault();
        break;
      }
    }
  });

  closeModalButton.on("click", function () {
    modalOverlay.css("visibility", "hidden");
  });

  modalOverlay.on("click", function (e) {
    if (e.target === modalOverlay[0]) modalOverlay.css("visibility", "hidden");
  });

  $("#functionSelect").on("change", function () {
    console.log("Selected function:", $(this).val());
    drawGraph();
  });

  $("#compileBtn").on("click", drawGraph);

  $("#analyzeBtn").on("click", function () {
    drawGraph();
  });

  $("#simulateBtn").on("click", function () {
    alert("Pipeline simulation (demo)");
  });

  $("#saveBtn").on("click", function () {
    alert("Save project (demo)");
  });

  $("#shareBtn").on("click", function () {
    alert("Share link (demo)");
  });

  $(window).on("resize", drawGraph);

  drawGraph();
});
