import TableView from "../game/TableView";
import SpectatorPanel from "./SpectatorPanel";
import HostControls from "./HostControls";
import EquityPanel from "./EquityPanel";
import ChatPanel from "./ChatPanel";

export default function LobbyView({
  snapshot,
  myUserId,
  isHost,
  gameInProgress,
  showStartNextHand,
  canStartNextHand,
  nextHandCountdownSec,
  onSeatClick,
  onHostAction,
  onSendChat
}) {
  const users = Array.isArray(snapshot?.users) ? snapshot.users : [];
  const game = snapshot?.game;
  const stage = game?.stage || "Idle";
  const hasPlayedHand = typeof game?.buttonPos === "number" && game.buttonPos >= 0;
  const viewer = users.find((u) => u.id === myUserId);
  const viewerIsSpectator = !!viewer?.isSpectator;
  const equities = snapshot?.equities;
  const chatMessages = Array.isArray(snapshot?.chatMessages)
    ? snapshot.chatMessages
    : [];
  const playersCount = users.filter((u) => !u.isSpectator).length;
  const spectatorsCount = users.length - playersCount;

  return (
    <div className="relative">
      <div className="lg:pr-[336px]">
        <TableView game={game} myUserId={myUserId} onSeatClick={onSeatClick} />
      </div>

      <div className="mt-4 space-y-4 lg:absolute lg:inset-y-0 lg:right-0 lg:mt-0 lg:w-80 lg:overflow-y-auto lg:pr-1">
        {isHost && (
          <HostControls
            isHost={isHost}
            users={users}
            stage={stage}
            gameInProgress={gameInProgress}
            hasPlayedHand={hasPlayedHand}
            showStartNextHand={showStartNextHand}
            canStartNextHand={canStartNextHand}
            nextHandCountdownSec={nextHandCountdownSec}
            lobbyConfig={snapshot?.lobbyConfig}
            onAction={onHostAction}
          />
        )}
        <details className="rounded-2xl border border-slate-700 bg-slate-900/50 p-3">
          <summary className="flex cursor-pointer list-none items-center justify-between text-sm font-semibold text-slate-200">
            <span>Room Members</span>
            <span className="text-xs text-slate-400">
              {playersCount} players / {spectatorsCount} spectators
            </span>
          </summary>
          <div className="mt-3">
            <SpectatorPanel users={users} embedded />
          </div>
        </details>
        {viewerIsSpectator && <EquityPanel game={game} equities={equities} />}
        <ChatPanel
          messages={chatMessages}
          myUserId={myUserId}
          onSend={onSendChat}
        />
      </div>
    </div>
  );
}
