import { useEffect, useMemo, useState } from "react";

function digitsOnly(value) {
  return value.replace(/[^\d]/g, "");
}

function parseOrFallback(text, fallback, { min = 0, max = 1_000_000 } = {}) {
  if (text === "") return fallback;
  const parsed = Number.parseInt(text, 10);
  if (Number.isNaN(parsed)) return fallback;
  return Math.min(Math.max(parsed, min), max);
}

function toConfigNumber(value, fallback) {
  return Number.isFinite(value) ? Number(value) : fallback;
}

export default function HostControls({
  isHost,
  users,
  stage,
  gameInProgress,
  hasPlayedHand,
  showStartNextHand,
  canStartNextHand,
  nextHandCountdownSec,
  lobbyConfig,
  onAction
}) {
  const usersList = Array.isArray(users) ? users : [];
  const kickCandidates = useMemo(
    () => usersList.filter((u) => !u.isHost),
    [usersList]
  );

  const configSmallBlind = toConfigNumber(lobbyConfig?.smallBlind, 5);
  const configBigBlind = toConfigNumber(lobbyConfig?.bigBlind, 10);
  const configStartingStack = toConfigNumber(lobbyConfig?.startingStack, 1000);
  const configMaxSeats = toConfigNumber(lobbyConfig?.maxSeats, 6);
  const configActionTimeout = toConfigNumber(lobbyConfig?.actionTimeout, 0);
  const configGodMode = typeof lobbyConfig?.godMode === "boolean" ? lobbyConfig.godMode : true;

  const [smallBlindInput, setSmallBlindInput] = useState(String(configSmallBlind));
  const [bigBlindInput, setBigBlindInput] = useState(String(configBigBlind));
  const [startingStackInput, setStartingStackInput] = useState(String(configStartingStack));
  const [maxSeatsInput, setMaxSeatsInput] = useState(String(configMaxSeats));
  const [actionTimeoutInput, setActionTimeoutInput] = useState(String(configActionTimeout));
  const [godMode, setGodMode] = useState(configGodMode);
  const [settingsOpen, setSettingsOpen] = useState(false);
  const [kickId, setKickId] = useState("");

  const showStartGame = stage === "Idle" && !gameInProgress;
  const showNextHand = stage === "Idle" && gameInProgress && showStartNextHand;
  const showEndGame = !!gameInProgress;
  const canEditConfig = stage === "Idle" && !gameInProgress && !!hasPlayedHand;

  const loadInputsFromConfig = () => {
    setSmallBlindInput(String(configSmallBlind));
    setBigBlindInput(String(configBigBlind));
    setStartingStackInput(String(configStartingStack));
    setMaxSeatsInput(String(configMaxSeats));
    setActionTimeoutInput(String(configActionTimeout));
    setGodMode(configGodMode);
  };

  useEffect(() => {
    if (!isHost) {
      if (settingsOpen) {
        setSettingsOpen(false);
      }
      return;
    }
    if (!canEditConfig && settingsOpen) {
      setSettingsOpen(false);
    }
  }, [isHost, canEditConfig, settingsOpen]);

  useEffect(() => {
    if (!isHost) return;
    if (!settingsOpen) return;
    loadInputsFromConfig();
  }, [
    isHost,
    settingsOpen,
    configSmallBlind,
    configBigBlind,
    configStartingStack,
    configMaxSeats,
    configActionTimeout,
    configGodMode
  ]);

  if (!isHost) return null;

  const submitConfig = () => {
    const smallBlind = parseOrFallback(smallBlindInput, configSmallBlind, { min: 1, max: 10000 });
    const bigBlindRaw = parseOrFallback(bigBlindInput, configBigBlind, { min: 1, max: 20000 });
    const bigBlind = Math.max(bigBlindRaw, smallBlind);
    const startingStack = parseOrFallback(startingStackInput, configStartingStack, { min: 1, max: 10_000_000 });
    const maxSeats = parseOrFallback(maxSeatsInput, configMaxSeats, { min: 2, max: 9 });
    const actionTimeout = parseOrFallback(actionTimeoutInput, configActionTimeout, { min: 0, max: 3600 });

    onAction("update_config", {
      smallBlind,
      bigBlind,
      startingStack,
      maxSeats,
      actionTimeout,
      godMode
    });
  };

  const startGameColSpan = showStartGame && (showNextHand || canEditConfig) ? "" : "col-span-2";
  const nextHandColSpan = showNextHand && (showStartGame || canEditConfig) ? "" : "col-span-2";
  const configColSpan = canEditConfig && (showStartGame || showNextHand) ? "" : "col-span-2";

  return (
    <div className="space-y-3 rounded-2xl border border-slate-700 bg-slate-900/70 p-4">
      <h3 className="text-sm font-semibold text-slate-200">Host Controls</h3>

      <div className="grid grid-cols-2 gap-2">
        {showStartGame && (
          <button
            className={`rounded bg-emerald-500 px-2 py-2 text-xs font-semibold text-emerald-950 ${startGameColSpan}`}
            onClick={() => onAction("start_game")}
          >
            Start Game
          </button>
        )}
        {showNextHand && (
          <button
            className={`rounded bg-blue-500 px-2 py-2 text-xs font-semibold text-blue-950 disabled:opacity-60 ${nextHandColSpan}`}
            disabled={!canStartNextHand}
            onClick={() => onAction("start_next_hand")}
          >
            {canStartNextHand
              ? "Start Next Hand"
              : `Reveal ${Math.max(nextHandCountdownSec, 1)}s`}
          </button>
        )}
        {canEditConfig && (
          <button
            className={`rounded border border-slate-500 bg-slate-800 px-2 py-2 text-xs font-semibold text-slate-100 ${configColSpan}`}
            onClick={() => setSettingsOpen((open) => !open)}
          >
            {settingsOpen ? "Close Config" : "Edit Config"}
          </button>
        )}
        {showEndGame && (
          <button className="col-span-2 rounded bg-rose-500 px-2 py-2 text-xs font-semibold text-rose-950" onClick={() => onAction("end_game")}>
            End Game
          </button>
        )}
      </div>

      {canEditConfig && settingsOpen && (
        <div className="rounded border border-slate-700 p-3">
          <div className="mb-2 text-xs uppercase text-slate-400">Game Settings</div>
          <div className="grid grid-cols-2 gap-2 text-xs">
            <label className="flex flex-col gap-1">
              Small Blind
              <input
                type="text"
                inputMode="numeric"
                value={smallBlindInput}
                onChange={(e) => setSmallBlindInput(digitsOnly(e.target.value))}
                className="rounded border border-slate-700 bg-slate-950 px-2 py-1"
              />
            </label>
            <label className="flex flex-col gap-1">
              Big Blind
              <input
                type="text"
                inputMode="numeric"
                value={bigBlindInput}
                onChange={(e) => setBigBlindInput(digitsOnly(e.target.value))}
                className="rounded border border-slate-700 bg-slate-950 px-2 py-1"
              />
            </label>
            <label className="flex flex-col gap-1">
              Starting Stack
              <input
                type="text"
                inputMode="numeric"
                value={startingStackInput}
                onChange={(e) => setStartingStackInput(digitsOnly(e.target.value))}
                className="rounded border border-slate-700 bg-slate-950 px-2 py-1"
              />
            </label>
            <label className="flex flex-col gap-1">
              Max Seats
              <input
                type="text"
                inputMode="numeric"
                value={maxSeatsInput}
                onChange={(e) => setMaxSeatsInput(digitsOnly(e.target.value))}
                className="rounded border border-slate-700 bg-slate-950 px-2 py-1"
              />
            </label>
            <label className="col-span-2 flex flex-col gap-1">
              Action Timeout
              <input
                type="text"
                inputMode="numeric"
                value={actionTimeoutInput}
                onChange={(e) => setActionTimeoutInput(digitsOnly(e.target.value))}
                className="rounded border border-slate-700 bg-slate-950 px-2 py-1"
              />
            </label>
            <label className="col-span-2 flex items-center gap-2">
              <input type="checkbox" checked={godMode} onChange={(e) => setGodMode(e.target.checked)} />
              God Mode (spectators see all cards)
            </label>
          </div>

          <button
            onClick={submitConfig}
            className="mt-3 w-full rounded bg-slate-200 px-2 py-2 text-xs font-semibold text-slate-900"
          >
            Apply Settings
          </button>
        </div>
      )}

      <div className="rounded border border-slate-700 p-3">
        <div className="mb-2 text-xs uppercase text-slate-400">Kick Player</div>
        <select
          value={kickId}
          onChange={(e) => setKickId(e.target.value)}
          className="mb-2 w-full rounded border border-slate-700 bg-slate-950 px-2 py-2 text-xs"
        >
          <option value="">Select player</option>
          {kickCandidates.map((u) => (
            <option key={u.id} value={u.id}>
              {u.name}
            </option>
          ))}
        </select>
        <button
          disabled={!kickId}
          onClick={() => onAction("kick_player", { targetId: kickId })}
          className="w-full rounded bg-rose-500 px-2 py-2 text-xs font-semibold text-rose-950 disabled:opacity-50"
        >
          Kick
        </button>
      </div>
    </div>
  );
}
