import { useEffect, useMemo, useRef, useState } from "react";

function formatTime(ts) {
  if (!Number.isFinite(ts)) return "";
  return new Date(ts).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
}

export default function ChatPanel({ messages, myUserId, onSend }) {
  const [text, setText] = useState("");
  const listRef = useRef(null);
  const rows = useMemo(() => (Array.isArray(messages) ? messages : []), [messages]);

  useEffect(() => {
    const el = listRef.current;
    if (!el) return;
    el.scrollTop = el.scrollHeight;
  }, [rows.length]);

  const submit = (e) => {
    e.preventDefault();
    const trimmed = text.trim();
    if (!trimmed) return;
    const sent = onSend?.(trimmed);
    if (sent) {
      setText("");
    }
  };

  return (
    <div className="rounded-2xl border border-slate-700 bg-slate-900/70 p-4">
      <h3 className="mb-2 text-sm font-semibold text-slate-200">Chat</h3>

      <div
        ref={listRef}
        className="mb-3 max-h-64 space-y-2 overflow-y-auto rounded border border-slate-800 bg-slate-950/70 p-2"
      >
        {rows.length === 0 && (
          <div className="text-xs text-slate-500">No messages yet.</div>
        )}

        {rows.map((m, idx) => {
          const mine = m?.userId === myUserId;
          return (
            <div
              key={m?.id || `${m?.timestamp || 0}_${idx}`}
              className={`rounded px-2 py-1.5 text-xs ${mine ? "bg-emerald-900/30" : "bg-slate-800/70"}`}
            >
              <div className="mb-0.5 flex items-center justify-between gap-2">
                <span className={`font-semibold ${mine ? "text-emerald-200" : "text-slate-200"}`}>
                  {m?.name || m?.userId || "Player"}
                </span>
                <span className="text-[10px] text-slate-500">{formatTime(m?.timestamp)}</span>
              </div>
              <div className="whitespace-pre-wrap break-words text-slate-200">{m?.text || ""}</div>
            </div>
          );
        })}
      </div>

      <form onSubmit={submit} className="flex gap-2">
        <input
          value={text}
          onChange={(e) => setText(e.target.value)}
          maxLength={280}
          placeholder="Type a message..."
          className="w-full rounded border border-slate-700 bg-slate-950 px-2 py-1.5 text-sm text-slate-100 outline-none focus:border-emerald-500"
        />
        <button
          type="submit"
          className="rounded bg-emerald-500 px-3 py-1.5 text-sm font-semibold text-emerald-950"
        >
          Send
        </button>
      </form>
    </div>
  );
}
