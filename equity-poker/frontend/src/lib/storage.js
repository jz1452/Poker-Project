const KEY = "equity_poker_session";

export function loadSession() {
  try {
    const raw = localStorage.getItem(KEY);
    if (!raw) return { userId: "", name: "" };
    const parsed = JSON.parse(raw);
    return {
      userId: typeof parsed.userId === "string" ? parsed.userId : "",
      name: typeof parsed.name === "string" ? parsed.name : ""
    };
  } catch {
    return { userId: "", name: "" };
  }
}

export function saveSession(userId, name) {
  try {
    localStorage.setItem(KEY, JSON.stringify({ userId, name }));
  } catch {
    // ignore storage failures
  }
}

export function clearSession() {
  try {
    localStorage.removeItem(KEY);
  } catch {
    // ignore storage failures
  }
}
