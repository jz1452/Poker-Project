const RANK_CHARS = ["2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"];
const SUIT_CHARS = ["c", "d", "h", "s"];

export function formatCardLabel(card) {
  if (!card || typeof card !== "object") return "??";
  if (typeof card.str === "string" && card.str.length >= 2) return card.str;

  const rank = Number(card.rank);
  const suit = Number(card.suit);
  const rankOk = Number.isInteger(rank) && rank >= 0 && rank < RANK_CHARS.length;
  const suitOk = Number.isInteger(suit) && suit >= 0 && suit < SUIT_CHARS.length;

  if (!rankOk || !suitOk) return "??";
  return `${RANK_CHARS[rank]}${SUIT_CHARS[suit]}`;
}
