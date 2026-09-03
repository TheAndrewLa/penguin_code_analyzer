import React from "react";
import { Share2 } from "lucide-react";

const Header: React.FC = () => {
    return (
        <header className="flex items-center justify-between px-6 h-16 bg-base-200 border-b border-base-300 flex-shrink-0">
            <div className="flex items-center gap-3">
                <img
                    src="/assets/logo.png"
                    alt="Penguin"
                    className="h-10 w-auto"
                />
                <div>
                    <div className="text-xl text-accent tracking-wide">
                        Penguin
                    </div>
                    <div className="text-xs font-medium text-neutral/60 tracking-widest uppercase">
                        code analyzer
                    </div>
                </div>
            </div>
            <button className="inline-flex items-center justify-center gap-2 px-3 py-1 text-lg font-medium text-gray-700 bg-gray-200 rounded-md hover:bg-gray-300">
                Share
                <Share2 size="1.125rem" />
            </button>
        </header>
    );
};

export default Header;
