import { execFile } from "node:child_process";
import { existsSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { createServer } from "node:http";
import { tmpdir } from "node:os";
import { basename, dirname, extname, join, normalize } from "node:path";
import { fileURLToPath } from "node:url";
import { promisify } from "node:util";

const execFileAsync = promisify(execFile);

const __dirname = dirname(fileURLToPath(import.meta.url));

const PORT = Number(process.env.BFF_PORT ?? 3000);
const INSTRUCTION_SERVICE = process.env.INSTRUCTION_SERVICE_URL ?? "http://localhost:8080";
const SIMULATE_SERVICE = process.env.SIMULATE_SERVICE_URL ?? "http://localhost:8090";
const GRAPH_SERVICE = process.env.GRAPH_SERVICE_URL ?? "http://localhost:9000";

const MAX_BODY_SIZE = 16 * 1024 * 1024;

const COMPILER_CONFIG = {
    "gcc 13.2": {
        cc: process.env.GCC_CC ?? "gcc",
    },
    "clang 22.0": {
        cc: process.env.CLANG_CC ?? "clang",
    },
};

const MIME_TYPES = {
    ".html": "text/html; charset=utf-8",
    ".js": "text/javascript; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".json": "application/json; charset=utf-8",
    ".png": "image/png",
    ".jpg": "image/jpeg",
    ".jpeg": "image/jpeg",
    ".svg": "image/svg+xml",
    ".ico": "image/x-icon",
    ".woff": "font/woff",
    ".woff2": "font/woff2",
    ".map": "application/json; charset=utf-8",
};

class HttpError extends Error {
    constructor(status, message) {
        super(message);
        this.status = status;
    }
}

async function readJsonBody(req) {
    const chunks = [];
    let size = 0;

    for await (const chunk of req) {
        size += chunk.length;
        if (size > MAX_BODY_SIZE) {
            throw new HttpError(413, "Request body too large");
        }
        chunks.push(chunk);
    }

    const raw = Buffer.concat(chunks).toString("utf8");
    if (!raw.trim()) return {};

    try {
        return JSON.parse(raw);
    } catch {
        throw new HttpError(400, "Invalid JSON body");
    }
}

async function parseResponse(response) {
    const text = await response.text();
    let body = null;

    try {
        body = text ? JSON.parse(text) : null;
    } catch {
        body = null;
    }

    if (!response.ok) {
        const message =
            body && typeof body === "object" && typeof body.error === "string"
                ? body.error
                : `Service responded with status ${response.status}`;
        throw new HttpError(response.status, message);
    }

    return body;
}

async function sendJson(url, payload) {
    return parseResponse(
        await fetch(url, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(payload),
        }),
    );
}

async function sendBinary(url, bytes) {
    return parseResponse(
        await fetch(url, {
            method: "POST",
            headers: { "Content-Type": "application/octet-stream" },
            body: bytes,
        }),
    );
}

async function sendGet(url) {
    return parseResponse(await fetch(url));
}

function splitFlags(flags) {
    return String(flags ?? "")
        .trim()
        .split(/\s+/)
        .filter((flag) => flag.length > 0);
}

const DEFAULT_SOURCE_NAME = "main.c";

// The tab name comes from a free-text input, so it must not be able to escape
// the temporary directory, and it must never start with "-" or the compiler
// would take it for an option instead of an input file.
function sanitizeSourceName(filename) {
    const stem = basename(String(filename ?? ""))
        .trim()
        .replace(/\.[^.]*$/, "");

    const safeStem = stem
        .replace(/[^A-Za-z0-9._-]/g, "_")
        .replace(/^[^A-Za-z0-9]+/, "")
        .slice(0, 64);

    return safeStem.length > 0 ? `${safeStem}.c` : DEFAULT_SOURCE_NAME;
}

function normalizeBrief(body) {
    const value = body ?? {};
    return {
        uOps: Number(value.uops ?? 0),
        latency: Number(value.latency ?? 0),
        rThroughput: Number(value.rthroughput ?? 0),
    };
}

function normalizeFull(body) {
    const value = body ?? {};
    return {
        ...normalizeBrief(body),
        bytes: Array.isArray(value.bytes) ? value.bytes.map(Number) : [],
        opcode: typeof value.opcode === "string" ? value.opcode : "",
    };
}

function writeJson(res, status, body) {
    const payload = JSON.stringify(body);
    res.writeHead(status, {
        "Content-Type": "application/json; charset=utf-8",
        "Content-Length": Buffer.byteLength(payload),
    });
    res.end(payload);
}

function writeFile(res, filePath) {
    const content = readFileSync(filePath);
    res.writeHead(200, {
        "Content-Type": MIME_TYPES[extname(filePath).toLowerCase()] ?? "application/octet-stream",
        "Content-Length": content.length,
    });
    res.end(content);
}

async function handleGraphStart(req, res) {
    const body = await readJsonBody(req);
    const { source, compiler, flags, filename } = body;

    if (typeof source !== "string" || source.length === 0) {
        writeJson(res, 400, { error: "Field 'source' is required" });
        return;
    }

    const toolchain = COMPILER_CONFIG[String(compiler ?? "")] ?? COMPILER_CONFIG["gcc 13.2"];
    const dir = mkdtempSync(join(tmpdir(), "penguin-compile-"));

    try {
        const srcName = sanitizeSourceName(filename);
        const objPath = join(dir, "out.o");
        writeFileSync(join(dir, srcName), source);

        const bin = toolchain.cc;
        const args = [];

        if (process.platform === "win32" && bin.includes("clang")) {
            args.push("--target=x86_64-unknown-linux-gnu");
        }

        args.push("-c", "-o", objPath, ...splitFlags(flags), srcName);

        try {
            // Compiling with cwd set to the temp directory keeps the temporary
            // path out of the diagnostics, so gcc reports just "main.c:2:12:".
            await execFileAsync(bin, args, { cwd: dir, maxBuffer: 32 * 1024 * 1024 });
        } catch (err) {
            const message = err?.stderr ? String(err.stderr) : err?.message ?? "Compilation failed";
            writeJson(res, 400, { error: message });
            return;
        }

        const objectBytes = readFileSync(objPath);
        const result = await sendBinary(`${GRAPH_SERVICE}/start`, objectBytes);
        writeJson(res, 200, result);
    } finally {
        rmSync(dir, { recursive: true, force: true });
    }
}

async function handleGraphStartBinary(req, res) {
    const chunks = [];
    let size = 0;

    for await (const chunk of req) {
        size += chunk.length;
        if (size > MAX_BODY_SIZE) {
            throw new HttpError(413, "Request body too large");
        }
        chunks.push(chunk);
    }

    const bytes = Buffer.concat(chunks);

    if (bytes.length === 0) {
        writeJson(res, 400, { error: "Binary body is empty" });
        return;
    }

    const result = await sendBinary(`${GRAPH_SERVICE}/start`, bytes);
    writeJson(res, 200, result);
}

async function handleGetGraph(req, res, url) {
    const session = url.searchParams.get("session");
    const name = url.searchParams.get("name");

    if (!session || !name) {
        writeJson(res, 400, { error: "Query parameters 'session' and 'name' are required" });
        return;
    }

    const body = await sendGet(
        `${GRAPH_SERVICE}/getGraph?session=${encodeURIComponent(session)}&name=${encodeURIComponent(name)}`,
    );
    writeJson(res, 200, body);
}

async function handleSimulateStart(req, res) {
    const body = await readJsonBody(req);
    const { cpu, instructions } = body;

    if (
        typeof cpu !== "string" ||
        !Array.isArray(instructions) ||
        !instructions.every((item) => typeof item === "string")
    ) {
        writeJson(res, 400, { error: "Fields 'cpu' and 'instructions' are required" });
        return;
    }

    const result = await sendJson(`${SIMULATE_SERVICE}/start`, { cpu, instructions });
    writeJson(res, 200, result);
}

async function handleSimulateResult(req, res, url, kind) {
    const session = url.searchParams.get("session");

    if (!session) {
        writeJson(res, 400, { error: "Query parameter 'session' is required" });
        return;
    }

    const path = {
        general: "getGeneral",
        instructions: "getInstructions",
        timeline: "getTimeline",
    }[kind];

    if (!path) {
        writeJson(res, 404, { error: "Unknown result kind" });
        return;
    }

    const body = await sendGet(`${SIMULATE_SERVICE}/${path}?session=${encodeURIComponent(session)}`);

    if (kind === "instructions") {
        const entries = Array.isArray(body)
            ? body.map(({ instrIndex, ...rest }) => rest)
            : [];
        writeJson(res, 200, entries);
        return;
    }

    writeJson(res, 200, body);
}

async function handleInstructionInfo(req, res, full) {
    const body = await readJsonBody(req);
    const { cpu, instruction } = body;

    if (typeof cpu !== "string" || typeof instruction !== "string") {
        writeJson(res, 400, { error: "Fields 'cpu' and 'instruction' are required" });
        return;
    }

    const endpoint = full ? "/full" : "/brief";
    const result = await sendJson(`${INSTRUCTION_SERVICE}${endpoint}`, { cpu, instruction });
    writeJson(res, 200, full ? normalizeFull(result) : normalizeBrief(result));
}

function serveStatic(req, res, pathname) {
    const distDir = join(__dirname, "../dist");
    const base = normalize(distDir);

    let filePath = normalize(join(distDir, pathname === "/" ? "index.html" : pathname));
    if (!filePath.startsWith(base)) {
        writeJson(res, 403, { error: "Forbidden" });
        return;
    }

    if (!existsSync(filePath)) {
        const index = join(distDir, "index.html");
        if (existsSync(index)) {
            filePath = index;
        } else {
            writeJson(res, 404, { error: "Not found" });
            return;
        }
    }

    writeFile(res, filePath);
}

const server = createServer(async (req, res) => {
    try {
        const method = req.method ?? "GET";
        const url = new URL(req.url ?? "/", `http://${req.headers.host ?? "localhost"}`);
        const path = url.pathname;

        if (method === "POST" && path === "/api/graph/start") {
            await handleGraphStart(req, res);
            return;
        }

        if (method === "POST" && path === "/api/graph/start-binary") {
            await handleGraphStartBinary(req, res);
            return;
        }

        if (method === "GET" && path === "/api/graph") {
            await handleGetGraph(req, res, url);
            return;
        }

        if (method === "POST" && path === "/api/simulate/start") {
            await handleSimulateStart(req, res);
            return;
        }

        if (method === "GET" && path === "/api/simulate/end") {
            const session = url.searchParams.get("session");

            if (!session) {
                writeJson(res, 400, { error: "Query parameter 'session' is required" });
                return;
            }

            const body = await sendGet(`${SIMULATE_SERVICE}/end?session=${encodeURIComponent(session)}`);
            writeJson(res, 200, body);
            return;
        }

        if (method === "GET" && path.startsWith("/api/simulate/")) {
            const kind = path.slice("/api/simulate/".length);
            await handleSimulateResult(req, res, url, kind);
            return;
        }

        if (method === "POST" && path === "/api/instruction/brief") {
            await handleInstructionInfo(req, res, false);
            return;
        }

        if (method === "POST" && path === "/api/instruction/full") {
            await handleInstructionInfo(req, res, true);
            return;
        }

        if (path.startsWith("/api/")) {
            writeJson(res, 404, { error: "Not found" });
            return;
        }

        serveStatic(req, res, path);
    } catch (err) {
        if (err instanceof HttpError) {
            writeJson(res, err.status, { error: err.message });
            return;
        }
        console.error(err);
        writeJson(res, 500, { error: err instanceof Error ? err.message : "Internal server error" });
    }
});

server.listen(PORT, () => {
    console.log(`BFF server listening on http://localhost:${PORT}`);
});
