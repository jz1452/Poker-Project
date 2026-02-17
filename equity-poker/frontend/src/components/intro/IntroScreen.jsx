import { useState } from "react";

export default function IntroScreen({ onJoin, isConnecting, defaultName }) {
  const [name, setName] = useState(defaultName || "");

  const handleSubmit = (e) => {
    e.preventDefault();
    onJoin(name);
  };

  return (
    <div className="flex min-h-screen items-center justify-center bg-gradient-to-b from-slate-950 via-slate-900 to-black p-6">
      <form
        onSubmit={handleSubmit}
        className="w-full max-w-md rounded-2xl border border-slate-700 bg-slate-900/80 p-8 shadow-2xl backdrop-blur"
      >
        <h1 className="mb-2 text-3xl font-semibold tracking-tight">Equity Poker</h1>
        <p className="mb-6 text-sm text-slate-400">Enter your name to join the room.</p>

        <label className="mb-2 block text-sm text-slate-300">Enter Your Name</label>
        <input
          value={name}
          onChange={(e) => setName(e.target.value)}
          placeholder="Player name"
          className="mb-4 w-full rounded-lg border border-slate-700 bg-slate-950 px-3 py-2 text-slate-100 outline-none ring-emerald-500 focus:ring"
        />

        <button
          type="submit"
          disabled={isConnecting}
          className="w-full rounded-lg bg-emerald-500 px-4 py-2 font-semibold text-emerald-950 transition hover:bg-emerald-400 disabled:cursor-not-allowed disabled:opacity-70"
        >
          {isConnecting ? "Connecting..." : "Join Room"}
        </button>
      </form>
    </div>
  );
}
