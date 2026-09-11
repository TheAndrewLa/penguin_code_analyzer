import React from "react";
import { Info } from "lucide-react";

interface HeaderProps {
    onShowInfo: () => void;
}

const Header: React.FC<HeaderProps> = ({ onShowInfo }) => {
    return (
        <header className="flex items-center justify-between px-6 h-16 bg-base-200 border-b border-base-300 flex-shrink-0">
            <div className="flex items-center gap-3">
                <img
                    src="/assets/logo.png"
                    alt="Penguin"
                    className="h-10 w-auto"
                />
                <div>
                    <div className="text-xl tracking-wider font-semibold text-black">
                        Penguin
                    </div>
                    <div className="text-xs font-medium tracking-widest uppercase font-mono text-gray-700">
                        code analyzer
                    </div>
                </div>
            </div>
            <button
                className="flex flex-row gap-2 items-center px-3 py-1 text-lg font-medium text-gray-700 bg-gray-200 rounded-md hover:bg-gray-300"
                onClick={onShowInfo}
            >
                Info
                <Info size="1.125rem" />
            </button>
        </header>
    );
};

export default Header;
