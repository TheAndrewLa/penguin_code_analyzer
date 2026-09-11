import React from "react";
import CodeMirror from "@uiw/react-codemirror";
import { cpp } from "@codemirror/lang-cpp";
import { javascript } from "@codemirror/lang-javascript";

interface EditorProps {
    value: string;
    onChange?: (value: string) => void;
    language?: "c" | "cpp" | "asm";
    readOnly?: boolean;
}

interface EditorLockedProps {
    instructions: string[];
}

export const Editor: React.FC<EditorProps> = ({
    value,
    onChange,
    language = "cpp",
    readOnly = false,
}) => {
    let extensions = [];
    if (language === "c" || language === "cpp") {
        extensions.push(cpp());
    } else if (language === "asm") {
        // Plain text for assembly
        // TODO: implement intel assembly syntax without extensions
        extensions.push(javascript({ jsx: false }));
    }

    return (
        <CodeMirror
            value={value}
            height="100%"
            extensions={extensions}
            onChange={onChange}
            theme="light"
            readOnly={readOnly}
            basicSetup={{
                lineNumbers: true,
                tabSize: 4,
                foldGutter: true,
                highlightActiveLine: true,
            }}
            className="h-full w-full"
        />
    );
};

export const EditorLocked: React.FC<EditorLockedProps> = ({ instructions }) => {
    return (
        <Editor
            value={instructions.join("\n")}
            onChange={() => {}}
            language="asm"
            readOnly={true}
        />
    );
};
