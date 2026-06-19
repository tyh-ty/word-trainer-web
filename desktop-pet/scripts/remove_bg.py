import sys
from collections import deque
from PIL import Image, ImageFilter
import numpy as np


def remove_background(input_path: str, output_path: str,
                      thresh: float = 40, smooth: float = 10) -> None:
    """Edge-aware background removal using gradient-guided flood-fill."""
    img = Image.open(input_path).convert("RGBA")
    arr = np.array(img, dtype=np.float32)
    h, w = arr.shape[:2]
    rgb = arr[:, :, :3]

    # --- 1. Edge detection (gradient magnitude) ---
    gray = np.mean(rgb, axis=2)
    gy = np.abs(np.diff(gray, axis=0, prepend=gray[:1, :]))
    gx = np.abs(np.diff(gray, axis=1, prepend=gray[:, :1]))
    grad = np.sqrt(gx**2 + gy**2)

    # --- 2. Distance to nearest edge ---
    edge = (grad > 12).astype(np.uint8)
    edge_dist = np.full((h, w), float(w + h), dtype=np.float32)
    q: deque = deque()
    for y in range(h):
        for x in range(w):
            if edge[y, x]:
                edge_dist[y, x] = 0
                q.append((y, x))
    dirs = [(-1, 0), (1, 0), (0, -1), (0, 1)]
    while q:
        y, x = q.popleft()
        nd = edge_dist[y, x] + 1
        for dy, dx in dirs:
            ny, nx = y + dy, x + dx
            if 0 <= ny < h and 0 <= nx < w and nd < edge_dist[ny, nx]:
                edge_dist[ny, nx] = nd
                q.append((ny, nx))

    # --- 3. Background color from corners ---
    edge_w = max(3, min(w, h) // 30)
    corners = np.concatenate([
        rgb[:edge_w, :edge_w].reshape(-1, 3),
        rgb[:edge_w, -edge_w:].reshape(-1, 3),
        rgb[-edge_w:, :edge_w].reshape(-1, 3),
        rgb[-edge_w:, -edge_w:].reshape(-1, 3),
    ], axis=0)
    bg_rgb = np.median(corners, axis=0)
    print(f"BG: R={bg_rgb[0]:.0f} G={bg_rgb[1]:.0f} B={bg_rgb[2]:.0f}")

    # --- 4. Color distance ---
    diff = rgb - bg_rgb
    cdist = np.sqrt(np.sum(diff * diff, axis=2))

    # --- 5. Flood from edges, with gradient gate ---
    visited = np.zeros((h, w), dtype=bool)
    for y in range(h):
        for x in range(w):
            on_edge = (y < edge_w or y >= h - edge_w or
                       x < edge_w or x >= w - edge_w)
            if on_edge and cdist[y, x] < thresh:
                visited[y, x] = True
                q.append((y, x))

    while q:
        y, x = q.popleft()
        for dy, dx in dirs:
            ny, nx = y + dy, x + dx
            if 0 <= ny < h and 0 <= nx < w and not visited[ny, nx]:
                # Stricter threshold near edges (protect character boundary)
                local_thresh = thresh if edge_dist[ny, nx] > 6 else thresh * 0.7
                if cdist[ny, nx] < local_thresh:
                    visited[ny, nx] = True
                    q.append((ny, nx))

    # --- 6. Build alpha ---
    alpha = np.ones((h, w), dtype=np.float32)
    for y in range(h):
        for x in range(w):
            if visited[y, x]:
                d = cdist[y, x]
                if d <= thresh - smooth:
                    alpha[y, x] = 0.0
                elif d < thresh:
                    alpha[y, x] = (thresh - d) / smooth
                else:
                    alpha[y, x] = 1.0

    arr[:, :, 3] = (alpha * 255).astype(np.uint8)
    Image.fromarray(arr.astype(np.uint8), "RGBA").save(output_path)
    print(f"Saved: {output_path}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python remove_bg.py input.jpg output.png")
        sys.exit(1)
    remove_background(sys.argv[1], sys.argv[2])
