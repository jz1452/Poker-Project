export default function Pot({ amount, stage }) {
  return (
    <div className="rounded-xl border border-slate-700 bg-black/30 px-4 py-2 text-center">
      <div className="text-xs uppercase tracking-wide text-slate-400">Stage</div>
      <div className="text-sm font-semibold text-slate-100">{stage}</div>
      <div className="mt-2 text-xs uppercase tracking-wide text-slate-400">Pot</div>
      <div className="text-xl font-bold text-emerald-300">{amount}</div>
    </div>
  );
}
