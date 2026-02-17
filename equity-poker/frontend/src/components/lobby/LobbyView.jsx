import TableView from "../game/TableView";
import SpectatorPanel from "./SpectatorPanel";
import HostControls from "./HostControls";
import EquityPanel from "./EquityPanel";

export default function LobbyView({
  snapshot,
  myUserId,
  isHost,
  gameInProgress,
  showStartNextHand,
  canStartNextHand,
  nextHandCountdownSec,
  onSeatClick,
  onHostAction
}) {
  const users = Array.isArray(snapshot?.users) ? snapshot.users : [];
  const game = snapshot?.game;
  const stage = game?.stage || "Idle";
  const hasPlayedHand = typeof game?.buttonPos === "number" && game.buttonPos >= 0;
  const viewer = users.find((u) => u.id === myUserId);
  const viewerIsSpectator = !!viewer?.isSpectator;
  const equities = snapshot?.equities;

  return (
    <div className="grid gap-4 lg:grid-cols-[1fr_320px]">
      <TableView game={game} myUserId={myUserId} onSeatClick={onSeatClick} />

      <div className="space-y-4">
        <SpectatorPanel users={users} />
        {viewerIsSpectator && <EquityPanel game={game} equities={equities} />}
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
      </div>
    </div>
  );
}
