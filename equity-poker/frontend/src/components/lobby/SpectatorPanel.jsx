export default function SpectatorPanel({ users }) {
  const usersList = Array.isArray(users) ? users : [];
  const spectators = usersList.filter((u) => u.isSpectator);
  const players = usersList.filter((u) => !u.isSpectator);

  return (
    <div className="rounded-2xl border border-slate-700 bg-slate-900/70 p-4">
      <h3 className="mb-3 text-sm font-semibold text-slate-200">Room Members</h3>

      <div className="mb-3">
        <div className="mb-1 text-xs uppercase text-slate-400">Players</div>
        <div className="space-y-1">
          {players.length === 0 && <div className="text-xs text-slate-500">No seated players</div>}
          {players.map((u) => (
            <div key={u.id} className="rounded bg-slate-800/60 px-2 py-1 text-xs">
              {u.name} {u.isHost ? "(Host)" : ""}
            </div>
          ))}
        </div>
      </div>

      <div>
        <div className="mb-1 text-xs uppercase text-slate-400">Spectators</div>
        <div className="space-y-1">
          {spectators.length === 0 && <div className="text-xs text-slate-500">No spectators</div>}
          {spectators.map((u) => (
            <div key={u.id} className="rounded bg-slate-800/60 px-2 py-1 text-xs">
              {u.name} {u.isHost ? "(Host)" : ""}
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
