import Seat from "./Seat";
import Board from "./Board";
import Pot from "./Pot";

function getTableLayout(totalSeats) {
  if (totalSeats >= 9) {
    return {
      radiusX: 46,
      radiusY: 39,
      seatVariant: "compact",
      tableHeightClass: "h-[740px]",
      ringInsetClass: "inset-[8%_3%]"
    };
  }

  if (totalSeats === 8) {
    return {
      radiusX: 45,
      radiusY: 37,
      seatVariant: "compact",
      tableHeightClass: "h-[710px]",
      ringInsetClass: "inset-[8%_4%]"
    };
  }

  if (totalSeats === 7) {
    return {
      radiusX: 45,
      radiusY: 36,
      seatVariant: "medium",
      tableHeightClass: "h-[680px]",
      ringInsetClass: "inset-[9%_5%]"
    };
  }

  return {
    radiusX: 43,
    radiusY: 34,
    seatVariant: "large",
    tableHeightClass: "h-[620px]",
    ringInsetClass: "inset-[10%_8%]"
  };
}

function getSeatPosition(index, totalSeats) {
  const layout = getTableLayout(totalSeats);
  const total = Math.max(totalSeats, 1);
  const angle = (-Math.PI / 2) + (2 * Math.PI * index) / total;
  const left = 50 + layout.radiusX * Math.cos(angle);
  const top = 50 + layout.radiusY * Math.sin(angle);

  return {
    left: `${left}%`,
    top: `${top}%`
  };
}

function getSeatPositionTags(game, seatIndex) {
  const tags = [];
  if (!game) return tags;
  if (game.buttonPos === seatIndex) tags.push("BTN");
  if (game.sbPos === seatIndex) tags.push("SB");
  if (game.bbPos === seatIndex) tags.push("BB");
  return tags;
}

export default function TableView({ game, myUserId, onSeatClick }) {
  if (!game) {
    return (
      <div className="rounded-3xl border border-slate-700 bg-slate-900/40 p-6 text-slate-300">
        Waiting for game state...
      </div>
    );
  }

  const seats = Array.isArray(game.seats) ? game.seats : [];
  const mySeatTaken = seats.some((seat) => seat?.id && seat.id === myUserId);
  const tableLayout = getTableLayout(seats.length);

  return (
    <div className="rounded-[40px] border border-emerald-950 bg-gradient-to-br from-felt-900 to-felt-700 p-6 shadow-table">
      <div className="mb-5 flex items-start justify-center gap-2 md:hidden">
        <div className="min-w-0 flex-1 overflow-x-auto">
          <Board cards={game.board} />
        </div>
        <Pot amount={game.pot} stage={game.stage} />
      </div>

      <div className="grid grid-cols-2 gap-3 lg:hidden">
        {seats.map((seat, index) => (
          <Seat
            key={index}
            seat={seat}
            index={index}
            positionTags={getSeatPositionTags(game, index)}
            sizeVariant={tableLayout.seatVariant}
            onSit={onSeatClick}
            canSit={!mySeatTaken}
            isCurrentActor={game.currentActor === index}
            isMe={seat?.id && seat.id === myUserId}
          />
        ))}
      </div>

      <div className={`relative hidden lg:block ${tableLayout.tableHeightClass}`}>
        <div className={`absolute rounded-[999px] border border-emerald-900/80 bg-gradient-to-b from-felt-800/70 to-felt-900/80 ${tableLayout.ringInsetClass}`} />
        <div className="absolute left-1/2 top-1/2 z-40 flex max-w-[92%] -translate-x-1/2 -translate-y-1/2 items-start justify-center gap-2">
          <Board cards={game.board} />
          <Pot amount={game.pot} stage={game.stage} />
        </div>

        {seats.map((seat, index) => (
          <div
            key={index}
            className="absolute z-20 -translate-x-1/2 -translate-y-1/2"
            style={getSeatPosition(index, seats.length)}
          >
            <Seat
              seat={seat}
              index={index}
              positionTags={getSeatPositionTags(game, index)}
              sizeVariant={tableLayout.seatVariant}
              onSit={onSeatClick}
              canSit={!mySeatTaken}
              isCurrentActor={game.currentActor === index}
              isMe={seat?.id && seat.id === myUserId}
            />
          </div>
        ))}
      </div>
    </div>
  );
}
