import CardFace from "./CardFace";

function statusBadge(status) {
  if (!status) return "";
  if (status === "Active") return "bg-emerald-600/90";
  if (status === "AllIn") return "bg-amber-600/90";
  if (status === "Folded") return "bg-slate-600/90";
  if (status === "Waiting") return "bg-sky-600/90";
  return "bg-slate-700/80";
}

function positionBadgeClass(tag) {
  if (tag === "BTN") return "bg-violet-500/90 text-violet-950";
  if (tag === "SB") return "bg-sky-500/90 text-sky-950";
  if (tag === "BB") return "bg-emerald-500/90 text-emerald-950";
  return "bg-slate-700/80 text-slate-100";
}

export default function Seat({
  seat,
  index,
  positionTags = [],
  sizeVariant = "large",
  isCurrentActor,
  isMe,
  onSit,
  canSit = true
}) {
  const sizeConfig = (() => {
    if (sizeVariant === "compact") {
      return {
        seatClass: "h-20 w-28 md:h-24 md:w-32",
        cardClass: "h-[3.25rem] w-[2.4rem] md:h-[3.75rem] md:w-[2.75rem]",
        cardOffsetClass: "-bottom-7 md:-bottom-8",
        cardAnchorClass: "left-[70%]",
        nameMaxClass: "max-w-[90px] md:max-w-[120px]"
      };
    }

    if (sizeVariant === "medium") {
      return {
        seatClass: "h-[5.5rem] w-[7.5rem] md:h-[6.5rem] md:w-[8.5rem]",
        cardClass: "h-[3.75rem] w-[2.75rem] md:h-[4.25rem] md:w-[3.15rem]",
        cardOffsetClass: "-bottom-8 md:-bottom-9",
        cardAnchorClass: "left-[72%]",
        nameMaxClass: "max-w-[100px] md:max-w-[130px]"
      };
    }

    return {
      seatClass: "h-24 w-32 md:h-28 md:w-36",
      cardClass: "h-[4.25rem] w-[3.15rem] md:h-[5.25rem] md:w-[3.75rem]",
      cardOffsetClass: "-bottom-9 md:-bottom-10",
      cardAnchorClass: "left-[74%]",
      nameMaxClass: "max-w-[110px] md:max-w-[140px]"
    };
  })();

  const empty = !seat?.id;
  const hand = Array.isArray(seat?.hand) ? seat.hand : [];
  const rawCardCount = Number(seat?.cardCount);
  const hiddenCount = Number.isInteger(rawCardCount)
    ? Math.max(rawCardCount - hand.length, 0)
    : 0;

  if (empty) {
    if (!canSit) {
      return (
        <div className={`flex cursor-not-allowed flex-col items-center justify-center rounded-xl border border-dashed border-slate-700 bg-slate-900/40 text-sm text-slate-500 ${sizeConfig.seatClass}`}>
          <span>Seat {index + 1}</span>
          <span className="mt-1 text-xs">Already seated</span>
        </div>
      );
    }

    return (
      <button
        onClick={() => onSit(index)}
        className={`flex flex-col items-center justify-center rounded-xl border border-dashed border-slate-500 bg-slate-800/30 text-sm text-slate-300 transition hover:border-emerald-400 hover:text-emerald-300 ${sizeConfig.seatClass}`}
      >
        <span>Seat {index + 1}</span>
        <span className="mt-1 text-xs">Click to Sit</span>
      </button>
    );
  }

  return (
    <div
      className={`relative flex flex-col overflow-visible rounded-xl border p-2 ${
        isMe ? "border-emerald-400 bg-emerald-900/20" : "border-slate-700 bg-slate-900/50"
      } ${isCurrentActor ? "ring-2 ring-amber-400" : ""} ${sizeConfig.seatClass}`}
    >
      <div className="mb-1 flex items-center justify-between">
        <span className={`truncate text-sm font-medium ${sizeConfig.nameMaxClass}`}>{seat.name || seat.id}</span>
        <span className={`rounded px-1.5 py-0.5 text-[10px] ${statusBadge(seat.status)}`}>
          {seat.status}
        </span>
      </div>
      <div className="text-xs text-slate-300">Chips: {seat.chips ?? 0}</div>
      <div className="text-xs text-slate-400">Bet: {seat.currentBet ?? 0}</div>
      {positionTags.length > 0 && (
        <div className="mt-1 flex flex-wrap gap-1">
          {positionTags.map((tag) => (
            <span
              key={tag}
              className={`rounded px-1.5 py-0.5 text-[10px] font-semibold ${positionBadgeClass(tag)}`}
            >
              {tag}
            </span>
          ))}
        </div>
      )}

      <div className={`absolute z-10 flex -translate-x-1/2 gap-1 ${sizeConfig.cardAnchorClass} ${sizeConfig.cardOffsetClass}`}>
        {hand.slice(0, 2).map((card, idx) => (
          <CardFace key={`visible_${idx}`} card={card} className={sizeConfig.cardClass} />
        ))}
        {Array.from({ length: hiddenCount }).map((_, idx) => (
          <CardFace key={`hidden_${idx}`} faceDown className={sizeConfig.cardClass} />
        ))}
      </div>
    </div>
  );
}
