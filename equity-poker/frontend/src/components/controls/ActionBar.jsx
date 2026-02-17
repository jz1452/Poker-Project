export default function ActionBar({
  visible,
  isMyTurn,
  canCheck,
  canCall,
  callAmount,
  raiseMin,
  raiseMax,
  raiseAmount,
  pending,
  onRaiseAmountChange,
  onAction,
  onStand,
  onLeave
}) {
  if (!visible) return null;

  const disableAct = !isMyTurn || pending;
  const canRaise = raiseMax >= raiseMin && raiseMin > 0;
  const sliderMin = Math.max(raiseMin || 0, 0);
  const sliderMax = Math.max(raiseMax || 0, 0);
  const safeMin = Math.min(sliderMin, sliderMax);
  const safeMax = Math.max(sliderMin, sliderMax);
  const safeValue = Math.min(Math.max(raiseAmount ?? safeMin, safeMin), safeMax);

  return (
    <div className="rounded-2xl border border-slate-700 bg-slate-900/80 p-4">
      <div className="mb-3 text-sm text-slate-300">Action Bar</div>

      <div className="mb-3 grid grid-cols-2 gap-2 md:grid-cols-4">
        <button
          onClick={() => onAction("fold")}
          disabled={disableAct}
          className="rounded bg-rose-500 px-3 py-2 text-sm font-medium text-rose-950 disabled:opacity-50"
        >
          Fold
        </button>

        <button
          onClick={() => onAction("check")}
          disabled={disableAct || !canCheck}
          className="rounded bg-slate-700 px-3 py-2 text-sm font-medium disabled:opacity-50"
        >
          Check
        </button>

        <button
          onClick={() => onAction("call")}
          disabled={disableAct || !canCall}
          className="rounded bg-blue-500 px-3 py-2 text-sm font-medium text-blue-950 disabled:opacity-50"
        >
          {canCall ? `Call ${callAmount}` : "Call"}
        </button>

        <button
          onClick={() => onAction("allin")}
          disabled={disableAct}
          className="rounded bg-amber-500 px-3 py-2 text-sm font-medium text-amber-950 disabled:opacity-50"
        >
          All-in
        </button>
      </div>

      <div className="mb-4 rounded border border-slate-700 p-3">
        <div className="mb-2 flex items-center justify-between text-xs text-slate-400">
          <span>Raise</span>
          <span>
            min {raiseMin} / max {raiseMax}
          </span>
        </div>
        <input
          type="range"
          min={safeMin}
          max={safeMax}
          step="1"
          value={safeValue}
          onChange={(e) => onRaiseAmountChange(Number(e.target.value))}
          disabled={disableAct || !canRaise}
          className="w-full"
        />
        <button
          onClick={() => onAction("raise", raiseAmount)}
          disabled={disableAct || !canRaise}
          className="mt-2 w-full rounded bg-emerald-500 px-3 py-2 text-sm font-semibold text-emerald-950 disabled:opacity-50"
        >
          Raise to {raiseAmount}
        </button>
      </div>

      <div className="grid grid-cols-2 gap-2">
        <button
          onClick={onStand}
          className="rounded border border-slate-600 bg-slate-800 px-3 py-2 text-sm"
        >
          Stand Up
        </button>
        <button
          onClick={onLeave}
          className="rounded border border-slate-600 bg-slate-800 px-3 py-2 text-sm"
        >
          Leave Room
        </button>
      </div>
    </div>
  );
}
