import React from "react";
import { clearSession } from "../lib/storage";

export default class AppErrorBoundary extends React.Component {
  constructor(props) {
    super(props);
    this.state = { hasError: false, message: "" };
  }

  static getDerivedStateFromError(error) {
    return {
      hasError: true,
      message: error?.message || "Unexpected rendering error."
    };
  }

  componentDidCatch(error, info) {
    console.error("App crashed:", error, info);
  }

  handleReset = () => {
    clearSession();
    window.location.reload();
  };

  render() {
    if (!this.state.hasError) {
      return this.props.children;
    }

    return (
      <div className="flex min-h-screen items-center justify-center bg-slate-950 p-6">
        <div className="w-full max-w-lg rounded-2xl border border-rose-700/60 bg-slate-900/80 p-6">
          <h1 className="mb-2 text-lg font-semibold text-rose-300">UI Render Error</h1>
          <p className="mb-3 text-sm text-slate-300">{this.state.message}</p>
          <p className="mb-5 text-xs text-slate-400">
            Session cache can sometimes hold stale state. Resetting clears local session data.
          </p>
          <button
            onClick={this.handleReset}
            className="rounded bg-rose-500 px-4 py-2 text-sm font-semibold text-rose-950"
          >
            Reset Session and Reload
          </button>
        </div>
      </div>
    );
  }
}
