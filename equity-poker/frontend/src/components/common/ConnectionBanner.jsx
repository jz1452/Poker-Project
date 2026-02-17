import { WifiOff } from "lucide-react";

export default function ConnectionBanner({ show, attempt }) {
  if (!show) return null;

  return (
    <div className="fixed left-1/2 top-3 z-50 flex -translate-x-1/2 items-center gap-2 rounded-full bg-amber-500/95 px-4 py-2 text-sm font-medium text-amber-950 shadow-lg">
      <WifiOff size={16} />
      Reconnecting{attempt > 0 ? ` (attempt ${attempt})` : ""}...
    </div>
  );
}
