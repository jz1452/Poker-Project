import { useEffect, useState } from "react";
import { formatCardLabel } from "../../lib/cardLabel";

function hasCardmeisterReady() {
  return typeof window !== "undefined" &&
    !!window.customElements &&
    !!window.customElements.get("playing-card");
}

export default function CardFace({ card, faceDown = false, className = "" }) {
  const cid = faceDown ? "00" : formatCardLabel(card);
  const classes = `card-face ${className}`.trim();
  const [ready, setReady] = useState(hasCardmeisterReady);

  useEffect(() => {
    if (ready) return;
    let raf = 0;

    const check = () => {
      if (hasCardmeisterReady()) {
        setReady(true);
        return;
      }
      raf = window.requestAnimationFrame(check);
    };

    check();
    return () => window.cancelAnimationFrame(raf);
  }, [ready]);

  return (
    <span className={classes} aria-label={faceDown ? "Face-down card" : `Card ${cid}`}>
      {ready ? <playing-card cid={cid}></playing-card> : <span className="card-fallback">{faceDown ? "[]": cid}</span>}
    </span>
  );
}
