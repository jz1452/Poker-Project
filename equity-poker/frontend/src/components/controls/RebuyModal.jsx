import { useState, useEffect } from "react";

export default function RebuyModal({
  open,
  defaultAmount,
  onClose,
  onConfirm,
  title = "Buy In",
  confirmLabel = "Confirm"
}) {
  const [amount, setAmount] = useState(defaultAmount || 1000);

  useEffect(() => {
    if (open) {
      setAmount(defaultAmount || 1000);
    }
  }, [open, defaultAmount]);

  if (!open) return null;

  return (
    <div className="fixed inset-0 z-40 flex items-center justify-center bg-black/60 p-4">
      <div className="w-full max-w-sm rounded-xl border border-slate-700 bg-slate-900 p-5">
        <h3 className="mb-4 text-lg font-semibold">{title}</h3>
        <input
          type="number"
          min="1"
          value={amount}
          onChange={(e) => setAmount(Number(e.target.value))}
          className="mb-4 w-full rounded border border-slate-700 bg-slate-950 px-3 py-2"
        />
        <div className="flex gap-2">
          <button
            onClick={() => onConfirm(amount)}
            className="flex-1 rounded bg-emerald-500 px-3 py-2 font-semibold text-emerald-950"
          >
            {confirmLabel}
          </button>
          <button
            onClick={onClose}
            className="flex-1 rounded border border-slate-600 bg-slate-800 px-3 py-2"
          >
            Cancel
          </button>
        </div>
      </div>
    </div>
  );
}
