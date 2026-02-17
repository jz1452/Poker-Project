export const selectConnection = (state) => state.connection;
export const selectSnapshot = (state) => state.snapshot;
export const selectUI = (state) => state.ui;
const EMPTY_ARRAY = Object.freeze([]);

export const selectUsers = (state) =>
  Array.isArray(state.snapshot?.users) ? state.snapshot.users : EMPTY_ARRAY;
export const selectGame = (state) => state.snapshot?.game || null;
export const selectSeats = (state) =>
  Array.isArray(state.snapshot?.game?.seats)
    ? state.snapshot.game.seats
    : EMPTY_ARRAY;
export const selectStage = (state) => state.snapshot?.game?.stage || "Idle";

export function selectMySeatIndex(state) {
  const userId = state.connection.userId;
  if (!userId) return -1;
  const seats = Array.isArray(state.snapshot?.game?.seats)
    ? state.snapshot.game.seats
    : EMPTY_ARRAY;
  return seats.findIndex((seat) => seat.id === userId);
}

export function selectMySeat(state) {
  const idx = selectMySeatIndex(state);
  if (idx < 0) return null;
  return state.snapshot?.game?.seats?.[idx] || null;
}

export function selectIsHost(state) {
  const userId = state.connection.userId;
  return !!userId && userId === state.snapshot?.hostId;
}

export function selectIsMyTurn(state) {
  const game = state.snapshot?.game;
  if (!game) return false;
  if (game.stage === "Idle" || game.stage === "Showdown") return false;
  const mySeatIndex = selectMySeatIndex(state);
  if (mySeatIndex < 0) return false;
  const seats = Array.isArray(game.seats) ? game.seats : EMPTY_ARRAY;
  const mySeat = seats[mySeatIndex];
  return game.currentActor === mySeatIndex && mySeat?.status === "Active";
}
