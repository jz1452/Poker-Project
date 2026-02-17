import { useEffect } from "react";

const kindClasses = {
  error: "bg-rose-600 text-white",
  info: "bg-slate-700 text-slate-100",
  success: "bg-emerald-600 text-white"
};

export default function Toasts({ toasts, removeToast }) {
  const toastList = Array.isArray(toasts) ? toasts : [];

  useEffect(() => {
    if (!toastList.length) return;

    const timers = toastList.map((toast) =>
      setTimeout(() => removeToast(toast.id), 3200)
    );

    return () => timers.forEach(clearTimeout);
  }, [toastList, removeToast]);

  return (
    <div className="fixed bottom-4 right-4 z-50 flex w-[320px] flex-col gap-2">
      {toastList.map((toast) => (
        <div
          key={toast.id}
          className={`rounded-lg px-3 py-2 text-sm shadow-lg ${kindClasses[toast.kind] || kindClasses.info}`}
        >
          {toast.message}
        </div>
      ))}
    </div>
  );
}
