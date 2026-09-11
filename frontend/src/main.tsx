import React from "react";
import ReactDOM from "react-dom/client";
import AppMock from "./AppMock";
import "./index.css";

ReactDOM.createRoot(document.getElementById("root")!).render(
    <React.StrictMode>
        <AppMock />
    </React.StrictMode>,
);
