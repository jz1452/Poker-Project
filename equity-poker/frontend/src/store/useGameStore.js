import { create } from "zustand";
import wsClient from "../network/wsClient";
import { clearSession, loadSession, saveSession } from "../lib/storage";

const initialSession = loadSession();

function pushToast(state, message, kind = "info") {
  const toast = {
    id: `${Date.now()}_${Math.random().toString(36).slice(2, 7)}`,
    message,
    kind
  };
  return [...state.toasts, toast].slice(-5);
}

export const useGameStore = create((set, get) => ({
  connection: {
    socketStatus: "disconnected",
    reconnectAttempt: 0,
    lastError: "",
    showReconnectBanner: false,
    userId: initialSession.userId,
    playerName: initialSession.name,
    hasJoined: false
  },
  snapshot: null,
  ui: {
    selectedSeat: null,
    buyInModalOpen: false,
    buyInAmount: 1000,
    raiseAmount: 0,
    pendingAction: null,
    toasts: []
  },

  setBuyInModal: (isOpen, seatIndex = null) => {
    set((state) => ({
      ui: {
        ...state.ui,
        buyInModalOpen: isOpen,
        selectedSeat: seatIndex
      }
    }));
  },

  setBuyInAmount: (amount) => {
    set((state) => ({
      ui: {
        ...state.ui,
        buyInAmount: amount
      }
    }));
  },

  setRaiseAmount: (amount) => {
    set((state) => {
      if (state.ui.raiseAmount === amount) {
        return state;
      }

      return {
        ui: {
          ...state.ui,
          raiseAmount: amount
        }
      };
    });
  },

  removeToast: (toastId) => {
    set((state) => ({
      ui: {
        ...state.ui,
        toasts: state.ui.toasts.filter((t) => t.id !== toastId)
      }
    }));
  },

  joinRoom: (name) => {
    const trimmed = name.trim();
    if (!trimmed) {
      set((state) => ({
        ui: {
          ...state.ui,
          toasts: pushToast(state.ui, "Name is required.", "error")
        }
      }));
      return;
    }

    const userId = get().connection.userId;
    wsClient.setJoinPayload({
      name: trimmed,
      id: userId || ""
    });
    wsClient.connect();

    set((state) => ({
      connection: {
        ...state.connection,
        playerName: trimmed,
        socketStatus: "connecting",
        lastError: ""
      }
    }));
  },

  sendAction: (action, payload = {}) => {
    const sent = wsClient.send({ action, ...payload });

    if (!sent) {
      set((state) => ({
        connection: {
          ...state.connection,
          lastError: "Not connected."
        },
        ui: {
          ...state.ui,
          toasts: pushToast(state.ui, "Action failed: socket not connected.", "error")
        }
      }));
      return false;
    }

    set((state) => ({
      ui: {
        ...state.ui,
        pendingAction: {
          action,
          startedAt: Date.now()
        }
      }
    }));

    return true;
  },

  sendChatMessage: (message) => {
    const trimmed = typeof message === "string" ? message.trim() : "";
    if (!trimmed) return false;

    const sent = wsClient.send({
      action: "chat",
      message: trimmed.slice(0, 280)
    });

    if (!sent) {
      set((state) => ({
        connection: {
          ...state.connection,
          lastError: "Not connected."
        },
        ui: {
          ...state.ui,
          toasts: pushToast(state.ui, "Chat failed: socket not connected.", "error")
        }
      }));
    }

    return sent;
  },

  leaveRoom: () => {
    wsClient.send({ action: "leave" });
    wsClient.setJoinPayload(null);
    wsClient.disconnect({ manual: true });
    clearSession();

    set((state) => ({
      connection: {
        ...state.connection,
        socketStatus: "disconnected",
        reconnectAttempt: 0,
        showReconnectBanner: false,
        userId: "",
        hasJoined: false
      },
      snapshot: null,
      ui: {
        ...state.ui,
        pendingAction: null,
        buyInModalOpen: false,
        selectedSeat: null
      }
    }));
  },

  initializeFromStorage: () => {
    const session = loadSession();
    if (!session.name) return;

    set((state) => ({
      connection: {
        ...state.connection,
        userId: session.userId,
        playerName: session.name,
        socketStatus: "connecting"
      }
    }));

    wsClient.setJoinPayload({
      name: session.name,
      id: session.userId || ""
    });
    wsClient.connect();
  },

  handleSocketOpen: () => {
    set((state) => ({
      connection: {
        ...state.connection,
        socketStatus: "connected",
        lastError: "",
        showReconnectBanner: false
      }
    }));
  },

  handleSocketClose: () => {
    const shouldReconnect = !!wsClient.joinPayload;
    set((state) => ({
      connection: {
        ...state.connection,
        socketStatus: shouldReconnect ? "reconnecting" : "disconnected",
        showReconnectBanner: shouldReconnect
      }
    }));
  },

  handleReconnectAttempt: (attempt) => {
    set((state) => ({
      connection: {
        ...state.connection,
        reconnectAttempt: attempt,
        socketStatus: "reconnecting",
        showReconnectBanner: true
      }
    }));
  },

  handleSocketError: (message) => {
    set((state) => ({
      connection: {
        ...state.connection,
        lastError: message
      },
      ui: {
        ...state.ui,
        toasts: pushToast(state.ui, message, "error")
      }
    }));
  },

  handleIncomingMessage: (msg) => {
    if (!msg || typeof msg !== "object") return;

    if (msg.type === "kicked") {
      const message = msg.message || "You were kicked from the room.";
      clearSession();
      wsClient.setJoinPayload(null);
      wsClient.disconnect({ manual: true });

      set((state) => ({
        connection: {
          ...state.connection,
          socketStatus: "disconnected",
          reconnectAttempt: 0,
          showReconnectBanner: false,
          userId: "",
          hasJoined: false,
          lastError: message
        },
        snapshot: null,
        ui: {
          ...state.ui,
          pendingAction: null,
          buyInModalOpen: false,
          selectedSeat: null,
          toasts: pushToast(state.ui, message, "error")
        }
      }));
      return;
    }

    if (msg.type === "joinSuccess") {
      const nextUserId = msg.userId || "";
      const currentName = get().connection.playerName;
      if (nextUserId) {
        saveSession(nextUserId, currentName);
      }
      set((state) => ({
        connection: {
          ...state.connection,
          userId: nextUserId,
          hasJoined: true,
          lastError: ""
        }
      }));
      return;
    }

    if (msg.type === "gameState") {
      if (!msg.data || typeof msg.data !== "object") {
        const message = "Received invalid game state payload.";
        set((state) => ({
          connection: {
            ...state.connection,
            lastError: message
          },
          ui: {
            ...state.ui,
            pendingAction: null,
            toasts: pushToast(state.ui, message, "error")
          }
        }));
        return;
      }

      set((state) => ({
        snapshot: msg.data,
        ui: {
          ...state.ui,
          pendingAction: null
        },
        connection: {
          ...state.connection,
          socketStatus: "connected",
          showReconnectBanner: false
        }
      }));
      return;
    }

    if (msg.type === "error") {
      const message = msg.message || "Server error.";
      set((state) => ({
        connection: {
          ...state.connection,
          lastError: message
        },
        ui: {
          ...state.ui,
          pendingAction: null,
          toasts: pushToast(state.ui, message, "error")
        }
      }));
    }
  }
}));
