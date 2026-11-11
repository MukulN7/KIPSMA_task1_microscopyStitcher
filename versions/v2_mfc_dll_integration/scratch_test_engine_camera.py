"""Temporary integration test: MicroStitchEngine with live USB microscope camera.

Verifies that MicroStitchEngine works with the exact type of frames that will
eventually be supplied by the MFC application via the C API bridge.

Controls
--------
  Q / ESC  - quit
  R        - reset the MicroStitchEngine

DO NOT COMMIT THIS FILE.
"""

import sys
import time

import cv2
import numpy as np

from microstitch.engine import MicroStitchEngine

# ── camera settings ─────────────────────────────────────────────────────────
FRAME_WIDTH  = 640
FRAME_HEIGHT = 360

# ── display settings ─────────────────────────────────────────────────────────
MOSAIC_DISPLAY_W = 640   # mosaic preview scaled to this width
STATUS_INTERVAL  = 30    # print status every N frames

# ── window names ─────────────────────────────────────────────────────────────
WIN_CAMERA = "Camera"
WIN_MOSAIC = "MicroStitch Mosaic"


def _resize_to_width(img: np.ndarray, target_w: int) -> np.ndarray:
    """Scale img to target_w preserving aspect ratio."""
    h, w = img.shape[:2]
    if w == 0:
        return img
    scale = target_w / w
    new_h = max(1, int(round(h * scale)))
    return cv2.resize(img, (target_w, new_h), interpolation=cv2.INTER_LINEAR)


def _to_u8_bgr(img: np.ndarray) -> np.ndarray:
    """Convert any dtype/channel layout to uint8 BGR for cv2.imshow."""
    if img.ndim == 2:
        # grayscale — normalise if not already uint8
        if img.dtype != np.uint8:
            mn, mx = float(img.min()), float(img.max())
            rng = mx - mn if mx != mn else 1.0
            img = ((img.astype(np.float32) - mn) / rng * 255).astype(np.uint8)
        img = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
    elif img.ndim == 3:
        if img.dtype != np.uint8:
            mn, mx = float(img.min()), float(img.max())
            rng = mx - mn if mx != mn else 1.0
            img = ((img.astype(np.float32) - mn) / rng * 255).astype(np.uint8)
    return img


def _overlay(img: np.ndarray, lines: list[str]) -> np.ndarray:
    """Draw text HUD onto a copy of img."""
    out = img.copy()
    y = 22
    for line in lines:
        cv2.putText(out, line, (8, y), cv2.FONT_HERSHEY_SIMPLEX,
                    0.5, (0, 0, 0), 2, cv2.LINE_AA)
        cv2.putText(out, line, (8, y), cv2.FONT_HERSHEY_SIMPLEX,
                    0.5, (255, 255, 255), 1, cv2.LINE_AA)
        y += 20
    return out


def main() -> None:
    # ── parse camera index from command line ─────────────────────────────────
    if len(sys.argv) < 2:
        print("Usage: python scratch_test_engine_camera.py <camera_index>")
        sys.exit(1)

    try:
        camera_index = int(sys.argv[1])
    except ValueError:
        print(f"[ERROR] Invalid camera index '{sys.argv[1]}'. Must be an integer.")
        sys.exit(1)

    # ── open camera ─────────────────────────────────────────────────────────
    cap = cv2.VideoCapture(camera_index)
    if not cap.isOpened():
        print(f"[ERROR] Cannot open camera index {camera_index}.", file=sys.stderr)
        sys.exit(1)

    cap.set(cv2.CAP_PROP_FRAME_WIDTH,  float(FRAME_WIDTH))
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, float(FRAME_HEIGHT))
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

    actual_w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    actual_h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    print(f"[INFO] Camera {camera_index} opened  →  {actual_w}x{actual_h}")
    print("[INFO] Controls:  Q / ESC = quit    R = reset engine")

    # ── engine ──────────────────────────────────────────────────────────────
    engine = MicroStitchEngine()

    frame_count  = 0
    mosaic_canvas: np.ndarray | None = None
    last_result: dict | None = None

    try:
        while True:
            ret, frame = cap.read()
            if not ret or frame is None:
                time.sleep(0.01)
                continue

            frame_count += 1

            # ── feed frame into the engine ───────────────────────────────────
            result = engine.process_frame(frame)
            last_result = result

            if result["mosaic"] is not None:
                mosaic_canvas = result["mosaic"]

            # ── periodic console status ──────────────────────────────────────
            if frame_count % STATUS_INTERVAL == 0:
                mos_shape = (
                    mosaic_canvas.shape if mosaic_canvas is not None else "None"
                )
                print(
                    f"[frame {frame_count:5d}] "
                    f"valid={result['registration_valid']}  "
                    f"dx={result['dx']:+7.2f}  dy={result['dy']:+7.2f}  "
                    f"q={result['quality_score']:.3f}  "
                    f"tiles={result['tile_count']}  "
                    f"mosaic={mos_shape}"
                )

            # ── camera window ────────────────────────────────────────────────
            cam_u8 = _to_u8_bgr(frame.copy())
            cam_hud = _overlay(cam_u8, [
                f"Camera {camera_index}  [{frame.shape[1]}x{frame.shape[0]}]",
                f"Tiles: {result['tile_count']}",
                f"valid={result['registration_valid']}  "
                f"dx={result['dx']:+.1f}  dy={result['dy']:+.1f}",
                f"quality={result['quality_score']:.3f}",
                "Q/ESC=quit   R=reset",
            ])
            cv2.imshow(WIN_CAMERA, cam_hud)

            # ── mosaic window ────────────────────────────────────────────────
            if mosaic_canvas is not None and mosaic_canvas.size > 0:
                mos_u8   = _to_u8_bgr(mosaic_canvas)
                mos_disp = _resize_to_width(mos_u8, MOSAIC_DISPLAY_W)
                mos_hud  = _overlay(mos_disp, [
                    f"MicroStitch Mosaic  tiles={result['tile_count']}",
                    f"{mosaic_canvas.shape}  {mosaic_canvas.dtype}",
                ])
                cv2.imshow(WIN_MOSAIC, mos_hud)

            # ── key handling ─────────────────────────────────────────────────
            key = cv2.waitKey(1) & 0xFF
            if key in (ord("q"), 27):       # Q or ESC
                print("[INFO] Quit.")
                break
            elif key == ord("r"):           # R
                engine.reset()
                mosaic_canvas = None
                print(f"[INFO] Engine reset at frame {frame_count}.")

    finally:
        cap.release()
        cv2.destroyAllWindows()
        if last_result is not None:
            print(
                f"\n[SUMMARY] Processed {frame_count} frames  "
                f"final tile_count={last_result['tile_count']}"
            )


if __name__ == "__main__":
    main()
