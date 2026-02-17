import CardFace from "./CardFace";

export default function Board({ cards }) {
  const boardCards = Array.isArray(cards) ? cards : [];

  return (
    <div className="flex min-h-10 items-center gap-2">
      {boardCards.length === 0 && <span className="text-sm text-slate-400">Board empty</span>}
      {boardCards.map((card, idx) => (
        <CardFace key={idx} card={card} className="h-24 w-16 md:h-32 md:w-24" />
      ))}
    </div>
  );
}
