import { useEffect } from "react";
import wsClient from "../network/wsClient";
import { useGameStore } from "../store/useGameStore";

export function useGameSocket() {
  const initializeFromStorage = useGameStore((s) => s.initializeFromStorage);
  const handleSocketOpen = useGameStore((s) => s.handleSocketOpen);
  const handleSocketClose = useGameStore((s) => s.handleSocketClose);
  const handleReconnectAttempt = useGameStore((s) => s.handleReconnectAttempt);
  const handleSocketError = useGameStore((s) => s.handleSocketError);
  const handleIncomingMessage = useGameStore((s) => s.handleIncomingMessage);

  useEffect(() => {
    wsClient.configure({
      onOpen: handleSocketOpen,
      onClose: handleSocketClose,
      onReconnectAttempt: handleReconnectAttempt,
      onError: handleSocketError,
      onMessage: handleIncomingMessage
    });

    initializeFromStorage();

    return () => {
      wsClient.disconnect({ manual: true });
    };
  }, [
    initializeFromStorage,
    handleSocketOpen,
    handleSocketClose,
    handleReconnectAttempt,
    handleSocketError,
    handleIncomingMessage
  ]);
}
