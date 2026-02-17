function formatEquity(value) {
  if (typeof value !== "number" || Number.isNaN(value)) {
    return "--";
  }
  const pct = value <= 1 ? value * 100 : value;
  return `${pct.toFixed(1)}%`;
}

export default function EquityPanel({ game, equities }) {
  if (!equities || typeof equities !== "object") return null;

  const rows = Object.entries(equities)
    .map(([seatIndex, value]) => {
      const index = Number(seatIndex);
      const seat = game?.seats?.[index];
      if (!seat?.id) return null;
      return {
        seatIndex: index,
        name: seat.name || seat.id,
        equity: value
      };
    })
    .filter(Boolean)
    .sort((a, b) => (b.equity || 0) - (a.equity || 0));

  if (!rows.length) return null;

  return (
    <div className="rounded-2xl border border-indigo-700/60 bg-indigo-950/30 p-4">
      <h3 className="mb-2 text-sm font-semibold text-indigo-200">Equity</h3>
      <div className="space-y-1.5">
        {rows.map((row) => (
          <div key={row.seatIndex} className="flex items-center justify-between rounded bg-slate-900/50 px-2 py-1.5 text-xs">
            <span className="truncate pr-2 text-slate-200">{row.name}</span>
            <span className="font-semibold text-indigo-200">{formatEquity(row.equity)}</span>
          </div>
        ))}
      </div>
    </div>
  );
}
