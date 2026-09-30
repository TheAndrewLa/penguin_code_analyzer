import * as Select from "@radix-ui/react-select";
import { ChevronDownIcon } from "lucide-react";

interface SelectFieldProps {
    value: string;
    onChange: (value: string) => void;
    options: string[];
    disabled?: boolean;
}

const SelectField: React.FC<SelectFieldProps> = ({
    value,
    onChange,
    options,
    disabled = false,
}) => {
    const sortedOptions = [value, ...options.filter((opt) => opt !== value)];
    return (
        <Select.Root value={value} onValueChange={onChange} disabled={disabled}>
            <Select.Trigger
                className={`inline-flex items-center justify-between gap-2 px-3 py-1 bg-white border border-gray-300 rounded-md text-base shadow-sm focus:outline-none focus:ring-1 focus:ring-blue-500 w-auto ${
                    disabled
                        ? "opacity-50 cursor-not-allowed"
                        : "hover:bg-gray-50"
                }`}
            >
                <Select.Value />
                <Select.Icon>
                    <ChevronDownIcon size={16} />
                </Select.Icon>
            </Select.Trigger>
            <Select.Portal>
                <Select.Content className="bg-white border border-gray-300 rounded-md shadow-md overflow-hidden z-50 max-h-60">
                    <Select.Viewport className="p-1">
                        {sortedOptions.map((opt) => (
                            <Select.Item
                                key={opt}
                                value={opt}
                                className="px-3 py-1 text-sm hover:bg-blue-50 rounded cursor-pointer"
                            >
                                <Select.ItemText>{opt}</Select.ItemText>
                            </Select.Item>
                        ))}
                    </Select.Viewport>
                </Select.Content>
            </Select.Portal>
        </Select.Root>
    );
};

export default SelectField;
