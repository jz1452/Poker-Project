function resolveDefaultWsUrl() {
  if (import.meta.env.VITE_WS_URL) {
    return import.meta.env.VITE_WS_URL;
  }

  if (typeof window !== "undefined" && window.location?.hostname) {
    const protocol = window.location.protocol === "https:" ? "wss" : "ws";
    return `${protocol}://${window.location.hostname}:9001`;
  }

  return "ws://localhost:9001";
}

const DEFAULT_URL = resolveDefaultWsUrl();

class WsClient {
  constructor() {
    this.url = DEFAULT_URL;
    this.ws = null;
    this.handlers = null;
    this.reconnectTimer = null;
    this.reconnectAttempt = 0;
    this.shouldReconnect = false;
    this.joinPayload = null;
  }

  configure(handlers) {
    this.handlers = handlers;
  }

  setJoinPayload(payload) {
    this.joinPayload = payload;
  }

  connect() {
    if (this.ws && (this.ws.readyState === WebSocket.OPEN || this.ws.readyState === WebSocket.CONNECTING)) {
      return;
    }

    this.shouldReconnect = true;
    this.ws = new WebSocket(this.url);

    this.ws.onopen = () => {
      this.reconnectAttempt = 0;
      this.handlers?.onOpen?.();

      if (this.joinPayload) {
        this.send({ action: "join", ...this.joinPayload });
      }
    };

    this.ws.onmessage = (event) => {
      try {
        const parsed = JSON.parse(event.data);
        this.handlers?.onMessage?.(parsed);
      } catch {
        this.handlers?.onError?.("Received invalid server message.");
      }
    };

    this.ws.onerror = () => {
      this.handlers?.onError?.("WebSocket error.");
    };

    this.ws.onclose = () => {
      this.ws = null;
      this.handlers?.onClose?.();

      if (!this.shouldReconnect || !this.joinPayload) {
        return;
      }

      const attempt = this.reconnectAttempt + 1;
      this.reconnectAttempt = attempt;
      const delay = Math.min(1000 * 2 ** (attempt - 1), 10000);
      this.handlers?.onReconnectAttempt?.(attempt, delay);

      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = setTimeout(() => {
        this.connect();
      }, delay);
    };
  }

  disconnect({ manual = true } = {}) {
    if (manual) {
      this.shouldReconnect = false;
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
  }

  send(message) {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      return false;
    }
    this.ws.send(JSON.stringify(message));
    return true;
  }
}

const wsClient = new WsClient();
export default wsClient;
