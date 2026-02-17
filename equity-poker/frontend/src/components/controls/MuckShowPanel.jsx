export default function MuckShowPanel({ visible, pending, onChoice }) {
  if (!visible) return null;

  return (
    <div className="rounded-2xl border border-amber-500/50 bg-amber-950/20 p-4">
      <div className="mb-2 text-sm font-semibold text-amber-200">Showdown Decision</div>
      <div className="mb-3 text-xs text-amber-100/80">
        Choose whether to reveal your cards.
      </div>
      <div className="grid grid-cols-2 gap-2">
        <button
          onClick={() => onChoice(false)}
          disabled={pending}
          className="rounded bg-slate-700 px-3 py-2 text-sm disabled:opacity-50"
        >
          Muck
        </button>
        <button
          onClick={() => onChoice(true)}
          disabled={pending}
          className="rounded bg-amber-500 px-3 py-2 text-sm font-semibold text-amber-950 disabled:opacity-50"
        >
          Show
        </button>
      </div>
    </div>
  );
}
