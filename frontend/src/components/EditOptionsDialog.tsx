import React, { useState } from "react";
import * as Dialog from "@radix-ui/react-dialog";
import { X } from "lucide-react";

interface EditOptionsDialogProps {
    isOpen: boolean;
    onClose: () => void;
    onAccept: (flags: string) => void;
    initialFlags?: string;
}

const EditOptionsDialog: React.FC<EditOptionsDialogProps> = ({
    isOpen,
    onClose,
    onAccept,
    initialFlags = "",
}) => {
    const [flags, setFlags] = useState(initialFlags);

    const handleAccept = () => {
        onAccept(flags);
        onClose();
    };

    return (
        <Dialog.Root open={isOpen} onOpenChange={(open) => !open && onClose()}>
            <Dialog.Portal>
                <Dialog.Overlay className="fixed inset-0 bg-black/50 z-50" />
                <Dialog.Content className="fixed top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 bg-white rounded-lg p-6 shadow-xl w-[90vw] max-w-md z-50">
                    <Dialog.Title className="text-lg font-bold">
                        Compilation Options
                    </Dialog.Title>
                    <Dialog.Description className="text-sm text-gray-500 mt-1">
                        Enter additional compiler flags
                    </Dialog.Description>
                    <div className="mt-4">
                        <input
                            type="text"
                            className="mt-1 block w-full rounded-md border border-gray-300 px-3 py-2 shadow-sm focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500"
                            value={flags}
                            onChange={(e) => setFlags(e.target.value)}
                            placeholder="-O2 -Wall"
                        />
                    </div>
                    <div className="mt-6 flex justify-end gap-2">
                        <button
                            className="px-4 py-2 text-sm font-medium text-gray-700 bg-gray-100 rounded-md hover:bg-gray-200"
                            onClick={onClose}
                        >
                            Cancel
                        </button>
                        <button
                            className="px-4 py-2 text-sm font-medium text-white bg-blue-600 rounded-md hover:bg-blue-700"
                            onClick={handleAccept}
                        >
                            Accept
                        </button>
                    </div>
                    <Dialog.Close asChild>
                        <button className="absolute top-2 right-2 p-1 text-gray-400 hover:text-gray-600">
                            <X strokeWidth={1.25} />
                        </button>
                    </Dialog.Close>
                </Dialog.Content>
            </Dialog.Portal>
        </Dialog.Root>
    );
};

export default EditOptionsDialog;
