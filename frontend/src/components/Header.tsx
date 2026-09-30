import React from "react";

interface HeaderProps {}

const Header: React.FC<HeaderProps> = () => {
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
        </header>
    );
};

export default Header;
