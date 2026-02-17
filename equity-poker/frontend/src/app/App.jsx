import { useEffect, useRef, useState } from "react";
import IntroScreen from "../components/intro/IntroScreen";
import LobbyView from "../components/lobby/LobbyView";
import ActionBar from "../components/controls/ActionBar";
import RebuyModal from "../components/controls/RebuyModal";
import MuckShowPanel from "../components/controls/MuckShowPanel";
import ConnectionBanner from "../components/common/ConnectionBanner";
import Toasts from "../components/common/Toasts";
import { useSocketLifecycle } from "../hooks/useSocketLifecycle";
import { useGameStore } from "../store/useGameStore";
import {
  selectGame,
  selectIsHost,
  selectIsMyTurn,
  selectMySeat,
  selectMySeatIndex,
  selectSnapshot,
  selectUsers
} from "../store/selectors";

const SHOWDOWN_REVEAL_MS = 5000;
const WINNER_BANNER_MS = 6500;
const EMPTY_USERS = Object.freeze([]);

function getSeatName(game, seatIndex) {
  const seats = Array.isArray(game?.seats) ? game.seats : [];
  const seat = seats[seatIndex];
  if (!seat) return null;
  return seat.name || seat.id || `Seat ${seatIndex + 1}`;
}

function formatNameList(names) {
  if (names.length <= 1) return names[0] || "";
  if (names.length === 2) return `${names[0]} and ${names[1]}`;
  return `${names.slice(0, -1).join(", ")}, and ${names[names.length - 1]}`;
}

function getTotalCommittedBets(game) {
  const seats = Array.isArray(game?.seats) ? game.seats : [];
  return seats.reduce((sum, seat) => sum + Math.max(Number(seat?.totalBet) || 0, 0), 0);
}

function getWinnerMessage(previousGame, currentGame) {
  const prevFoldWinner = Number(previousGame?.foldWinner);
  if (Number.isInteger(prevFoldWinner) && prevFoldWinner >= 0) {
    const winnerName = getSeatName(previousGame, prevFoldWinner) || getSeatName(currentGame, prevFoldWinner);
    if (winnerName) {
      const won = getTotalCommittedBets(previousGame);
      return won > 0 ? `${winnerName} wins ${won}` : `${winnerName} wins the hand`;
    }
  }

  const currentResults = Array.isArray(currentGame?.showdownResults) ? currentGame.showdownResults : [];
  const previousResults = Array.isArray(previousGame?.showdownResults) ? previousGame.showdownResults : [];
  const results = currentResults.length ? currentResults : previousResults;
  if (!results.length) return "";

  const winners = [];
  const winnerRows = [];
  for (const row of results) {
    if (!row || row.chipsWon <= 0 || !Number.isInteger(row.seatIndex)) continue;
    const winnerName = getSeatName(currentGame, row.seatIndex) || getSeatName(previousGame, row.seatIndex);
    if (winnerName && !winners.includes(winnerName)) {
      winners.push(winnerName);
    }
    if (winnerName) {
      winnerRows.push({ name: winnerName, chipsWon: row.chipsWon });
    }
  }

  if (winnerRows.length === 1) {
    return `${winnerRows[0].name} wins ${winnerRows[0].chipsWon}`;
  }
  if (winnerRows.length > 1) {
    const everyoneSame = winnerRows.every((row) => row.chipsWon === winnerRows[0].chipsWon);
    if (everyoneSame) {
      return `${formatNameList(winners)} split the pot (${winnerRows[0].chipsWon} each)`;
    }
    const detail = winnerRows.map((row) => `${row.name} +${row.chipsWon}`).join(", ");
    return `Split pot: ${detail}`;
  }
  return "";
}

export default function App() {
  useSocketLifecycle();
  const [rebuyModalOpen, setRebuyModalOpen] = useState(false);
  const [nextHandUnlockAt, setNextHandUnlockAt] = useState(0);
  const [clockNow, setClockNow] = useState(Date.now());
  const [winnerBannerText, setWinnerBannerText] = useState("");
  const prevStageRef = useRef(null);
  const previousGameRef = useRef(null);
  const winnerBannerTimerRef = useRef(null);

  const connection = useGameStore((s) => s.connection);
  const ui = useGameStore((s) => s.ui);
  const snapshot = useGameStore(selectSnapshot);
  const users = useGameStore(selectUsers);
  const game = useGameStore(selectGame);
  const mySeat = useGameStore(selectMySeat);
  const mySeatIndex = useGameStore(selectMySeatIndex);
  const isHost = useGameStore(selectIsHost);
  const isMyTurn = useGameStore(selectIsMyTurn);

  const joinRoom = useGameStore((s) => s.joinRoom);
  const sendAction = useGameStore((s) => s.sendAction);
  const leaveRoom = useGameStore((s) => s.leaveRoom);
  const setBuyInModal = useGameStore((s) => s.setBuyInModal);
  const setRaiseAmount = useGameStore((s) => s.setRaiseAmount);
  const removeToast = useGameStore((s) => s.removeToast);

  const stage = game?.stage || "Idle";
  const stageIsIdle = stage === "Idle";
  const gameInProgress = !!snapshot?.isGameInProgress;
  const usersList = Array.isArray(users) ? users : EMPTY_USERS;
  const viewer = usersList.find((u) => u.id === connection.userId);
  const viewerIsSpectator = !!viewer?.isSpectator;
  const callAmount = Math.max((game?.currentBet || 0) - (mySeat?.currentBet || 0), 0);
  const canCheck = callAmount === 0;
  const canCall = callAmount > 0 && (mySeat?.chips || 0) > 0;

  const raiseMin = (game?.currentBet || 0) + (game?.minRaise || 0);
  const raiseMax = (mySeat?.currentBet || 0) + (mySeat?.chips || 0);

  useEffect(() => {
    if (!mySeat) return;
    const safeRaiseMin = Math.max(raiseMin || 0, 0);
    const safeRaiseMax = Math.max(raiseMax || 0, 0);

    // If raising is not possible for this player state, keep slider value bounded
    // without creating a render loop.
    if (safeRaiseMax < safeRaiseMin) {
      if (ui.raiseAmount !== safeRaiseMax) {
        setRaiseAmount(safeRaiseMax);
      }
      return;
    }

    const current = Number.isFinite(ui.raiseAmount) ? ui.raiseAmount : safeRaiseMin;
    const clamped = Math.min(Math.max(current, safeRaiseMin), safeRaiseMax);
    if (clamped !== ui.raiseAmount) {
      setRaiseAmount(clamped);
    }
  }, [mySeat, ui.raiseAmount, raiseMin, raiseMax, setRaiseAmount]);

  useEffect(() => {
    const previousStage = prevStageRef.current;
    if (previousStage === "Showdown" && stage === "Idle") {
      const unlockAt = Date.now() + SHOWDOWN_REVEAL_MS;
      setNextHandUnlockAt(unlockAt);
      setClockNow(Date.now());
    }
    prevStageRef.current = stage;
  }, [stage]);

  useEffect(() => {
    if (!nextHandUnlockAt) return;

    const interval = setInterval(() => {
      const now = Date.now();
      setClockNow(now);
      if (now >= nextHandUnlockAt) {
        setNextHandUnlockAt(0);
      }
    }, 250);

    return () => clearInterval(interval);
  }, [nextHandUnlockAt]);

  useEffect(() => {
    if (!connection.hasJoined) {
      previousGameRef.current = null;
      setWinnerBannerText("");
      if (winnerBannerTimerRef.current) {
        clearTimeout(winnerBannerTimerRef.current);
        winnerBannerTimerRef.current = null;
      }
      return;
    }

    const previousGame = previousGameRef.current;
    const previousStage = previousGame?.stage || "Idle";
    const currentStage = game?.stage || "Idle";

    if (previousGame && previousStage !== "Idle" && currentStage === "Idle") {
      const message = getWinnerMessage(previousGame, game);
      if (message) {
        setWinnerBannerText(message);
        if (winnerBannerTimerRef.current) {
          clearTimeout(winnerBannerTimerRef.current);
        }
        winnerBannerTimerRef.current = setTimeout(() => {
          setWinnerBannerText("");
          winnerBannerTimerRef.current = null;
        }, WINNER_BANNER_MS);
      }
    }

    previousGameRef.current = game || null;
  }, [connection.hasJoined, game]);

  useEffect(() => {
    return () => {
      if (winnerBannerTimerRef.current) {
        clearTimeout(winnerBannerTimerRef.current);
      }
    };
  }, []);

  const hasOptionalShowdownDecision = (() => {
    if (!game || mySeatIndex < 0) return false;

    if ((game.foldWinner ?? -1) === mySeatIndex) {
      return game.stage !== "Idle";
    }

    if (game.stage !== "Showdown") {
      return false;
    }

    const showdownResults = Array.isArray(game.showdownResults) ? game.showdownResults : [];
    const myResult = showdownResults.find((r) => r.seatIndex === mySeatIndex);
    if (!myResult) return false;

    return !myResult.mustShow && !myResult.hasDecided;
  })();

  const revealCountdownMs = Math.max(nextHandUnlockAt - clockNow, 0);
  const nextHandCountdownSec = Math.ceil(revealCountdownMs / 1000);
  const showStartNextHand = stageIsIdle && gameInProgress;
  const canStartNextHand = showStartNextHand && revealCountdownMs === 0;
  const canRebuy = !!mySeat && stageIsIdle;

  if (!connection.hasJoined) {
    return (
      <>
        <ConnectionBanner show={connection.showReconnectBanner} attempt={connection.reconnectAttempt} />
        <IntroScreen
          onJoin={joinRoom}
          isConnecting={connection.socketStatus === "connecting" || connection.socketStatus === "reconnecting"}
          defaultName={connection.playerName}
        />
        <Toasts toasts={ui.toasts} removeToast={removeToast} />
      </>
    );
  }

  return (
    <div className="min-h-screen p-4 md:p-6">
      <ConnectionBanner show={connection.showReconnectBanner} attempt={connection.reconnectAttempt} />
      {winnerBannerText && (
        <div className="pointer-events-none fixed left-1/2 top-1/2 z-[120] -translate-x-1/2 -translate-y-1/2 rounded-2xl border border-emerald-500/70 bg-slate-900/90 px-6 py-4 text-center shadow-2xl">
          <div className="text-[11px] uppercase tracking-[0.15em] text-emerald-300/80">Hand Complete</div>
          <div className="mt-1 text-lg font-semibold text-emerald-200">{winnerBannerText}</div>
        </div>
      )}

      <header className="mb-4 flex flex-wrap items-center justify-between gap-2 rounded-xl border border-slate-800 bg-slate-900/70 p-3">
        <div>
          <div className="text-sm text-slate-300">
            Connected as <span className="font-semibold text-slate-100">{connection.playerName}</span>
          </div>
          <div className="text-xs text-slate-500">Status: {connection.socketStatus}</div>
        </div>

        <div className="flex items-center gap-2">
          {canRebuy && (
            <button
              onClick={() => setRebuyModalOpen(true)}
              className="rounded bg-emerald-500 px-3 py-1.5 text-sm font-semibold text-emerald-950"
            >
              Add Chips
            </button>
          )}
          <button onClick={leaveRoom} className="rounded border border-slate-600 bg-slate-800 px-3 py-1.5 text-sm">
            Leave Room
          </button>
        </div>
      </header>

      <LobbyView
        snapshot={snapshot}
        myUserId={connection.userId}
        isHost={isHost}
        gameInProgress={gameInProgress}
        showStartNextHand={showStartNextHand}
        canStartNextHand={canStartNextHand}
        nextHandCountdownSec={nextHandCountdownSec}
        onSeatClick={(seatIndex) => {
          if (mySeat) return;
          setBuyInModal(true, seatIndex);
        }}
        onHostAction={(action, payload = {}) => sendAction(action, payload)}
      />

      <div className="mt-4 grid gap-4 lg:grid-cols-[1fr_320px]">
        <ActionBar
          visible={!!mySeat}
          isMyTurn={isMyTurn}
          canCheck={canCheck}
          canCall={canCall}
          callAmount={callAmount}
          raiseMin={raiseMin}
          raiseMax={raiseMax}
          raiseAmount={ui.raiseAmount || raiseMin || 0}
          onRaiseAmountChange={setRaiseAmount}
          onAction={(command, amount) => sendAction("game_action", { command, ...(amount ? { amount } : {}) })}
          pending={!!ui.pendingAction}
          onStand={() => sendAction("stand")}
          onLeave={leaveRoom}
        />

        <MuckShowPanel
          visible={hasOptionalShowdownDecision}
          pending={!!ui.pendingAction}
          onChoice={(show) => sendAction("muck_show", { show })}
        />
      </div>

      <RebuyModal
        open={ui.buyInModalOpen}
        defaultAmount={ui.buyInAmount}
        title="Buy In"
        confirmLabel="Sit Down"
        onClose={() => setBuyInModal(false, null)}
        onConfirm={(amount) => {
          if (ui.selectedSeat == null) return;
          sendAction("sit", { seatIndex: ui.selectedSeat, buyIn: amount });
          setBuyInModal(false, null);
        }}
      />

      <RebuyModal
        open={rebuyModalOpen}
        defaultAmount={snapshot?.lobbyConfig?.startingStack || 500}
        title="Rebuy Chips"
        confirmLabel="Add Chips"
        onClose={() => setRebuyModalOpen(false)}
        onConfirm={(amount) => {
          if (amount > 0) {
            sendAction("rebuy", { amount });
          }
          setRebuyModalOpen(false);
        }}
      />

      <Toasts toasts={ui.toasts} removeToast={removeToast} />

      {viewerIsSpectator && (
        <div className="mt-3 text-xs text-slate-400">You are currently spectating.</div>
      )}
    </div>
  );
}
