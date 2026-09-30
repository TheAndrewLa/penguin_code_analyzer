import React from "react";
import CodeMirror from "@uiw/react-codemirror";
import { cpp } from "@codemirror/lang-cpp";

interface EditorProps {
    value: string;
    onChange?: (value: string) => void;
    readOnly?: boolean;
}

export const Editor: React.FC<EditorProps> = ({
    value,
    onChange,
    readOnly = false,
}) => {
    return (
        <CodeMirror
            value={value}
            height="100%"
            extensions={[cpp()]}
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