"""Generate the G-AcidBase logo assets from one shared geometry spec.

The same normalised geometry is drawn by Source/Logo.h inside the plugin, so the
vector mark in the editor, the shipped PNG/SVG and the application icon match.
Output: assets/G-AcidBase-Logo.svg plus PNG rasters at 64/128/256/512/1024 px.

Pure standard library: PNGs are written with zlib and evaluated with
per-subsample boolean tests, so output is deterministic and dependency-free.
"""
from pathlib import Path
import math
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'assets'

BADGE_FILL = (0x10, 0x16, 0x1f)
BADGE_EDGE = (0x33, 0x42, 0x5a)
WAVE = (0xb8, 0xff, 0x38)
SLOPE = (0x42, 0xe8, 0xda)
KNOB = (0xb2, 0x90, 0xff)

# Normalised geometry (0..1 square). Keep in sync with Source/Logo.h.
BADGE_INSET = 0.02
BADGE_RADIUS = 0.22
BADGE_EDGE_WIDTH = 0.035
WAVE_WIDTH = 0.05
WAVE_BASELINE = 0.44
WAVE_PEAK = 0.19
WAVE_LEFT = 0.13
WAVE_RIGHT = 0.87
WAVE_CYCLES = 3
SLOPE_WIDTH = 0.038
SLOPE_START = (0.13, 0.54)
SLOPE_END = (0.87, 0.72)
SLOPE_DOT_RADIUS = 0.055
KNOB_CENTRE = (0.22, 0.79)
KNOB_RADIUS = 0.088
KNOB_RING_WIDTH = 0.036
KNOB_NOTCH = (0.282, 0.728)
KNOB_NOTCH_WIDTH = 0.042


def wave_points():
    span = (WAVE_RIGHT - WAVE_LEFT) / WAVE_CYCLES
    points = [(WAVE_LEFT, WAVE_BASELINE)]
    for cycle in range(WAVE_CYCLES):
        end = WAVE_LEFT + span * (cycle + 1)
        points.append((end - 0.02, WAVE_PEAK))
        points.append((end - 0.02, WAVE_BASELINE))
    return points


def slope_points():
    return [SLOPE_START, SLOPE_END]


def distance_to_segment(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    length = dx * dx + dy * dy
    t = 0.0 if length == 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / length))
    return math.hypot(px - (ax + dx * t), py - (ay + dy * t))


def rounded_rect_distance(px, py, inset, radius):
    """Signed distance to the rounded badge outline (negative inside)."""
    half = 0.5 - inset
    cx, cy = 0.5, 0.5
    qx = abs(px - cx) - (half - radius)
    qy = abs(py - cy) - (half - radius)
    outside = math.hypot(max(qx, 0.0), max(qy, 0.0))
    return outside + min(max(qx, qy), 0.0) - radius


class Raster:
    def __init__(self, size, samples=3):
        self.size = size
        self.samples = samples
        self.pixels = [[0, 0, 0, 0] for _ in range(size * size)]
        self.scale = size
        self.offsets = [(index + 0.5) / samples for index in range(samples)]

    def _blend(self, index, colour, coverage):
        # Straight (non-premultiplied) alpha source-over compositing.
        if coverage <= 0.0:
            return
        source_alpha = (colour[3] / 255.0) if len(colour) > 3 else 1.0
        alpha = coverage * source_alpha
        if alpha <= 0.0:
            return
        pixel = self.pixels[index]
        destination_alpha = pixel[3] / 255.0
        output_alpha = alpha + destination_alpha * (1 - alpha)
        for channel in range(3):
            mixed = (colour[channel] * alpha + pixel[channel] * destination_alpha * (1 - alpha)) / output_alpha
            pixel[channel] = int(round(max(0.0, min(255.0, mixed))))
        pixel[3] = int(round(max(0.0, min(255.0, output_alpha * 255))))

    def _for_each_sample(self, bounds, test, colour):
        # bounds are normalised; convert to pixel space before iterating
        left, top, right, bottom = (v * self.scale for v in bounds)
        x0, y0 = max(0, int(math.floor(left))), max(0, int(math.floor(top)))
        x1 = min(self.size - 1, int(math.floor(right)))
        y1 = min(self.size - 1, int(math.floor(bottom)))
        if x1 < x0 or y1 < y0:
            raise ValueError(f'primitive outside canvas: {bounds}')
        count = self.samples * self.samples
        for y in range(y0, y1 + 1):
            row = y * self.size
            for x in range(x0, x1 + 1):
                hits = 0
                for sub_y in self.offsets:
                    ny = (y + sub_y) / self.scale
                    for sub_x in self.offsets:
                        if test((x + sub_x) / self.scale, ny):
                            hits += 1
                if hits:
                    self._blend(row + x, colour, hits / count)

    def fill_rounded_rect(self, colour):
        self._for_each_sample((-0.02, -0.02, 1.02, 1.02),
                              lambda x, y: rounded_rect_distance(x, y, BADGE_INSET, BADGE_RADIUS) <= 0.0,
                              colour)

    def stroke_rounded_rect(self, colour):
        half = BADGE_EDGE_WIDTH / 2.0
        self._for_each_sample((-0.02, -0.02, 1.02, 1.02),
                              lambda x, y: abs(rounded_rect_distance(x, y, BADGE_INSET, BADGE_RADIUS)) <= half,
                              colour)

    def stroke_polyline(self, points, width, colour, dot_radius=None):
        pad = width + 0.03
        bounds = (min(p[0] for p in points) - pad, min(p[1] for p in points) - pad,
                  max(p[0] for p in points) + pad, max(p[1] for p in points) + pad)
        half = width / 2.0
        if dot_radius:
            self._for_each_sample(bounds, lambda x, y: math.hypot(x - points[-1][0], y - points[-1][1]) <= dot_radius, colour)

        def test(x, y):
            for index in range(len(points) - 1):
                if distance_to_segment(x, y, *points[index], *points[index + 1]) <= half:
                    return True
            return False

        self._for_each_sample(bounds, test, colour)

    def fill_circle(self, centre, radius, colour):
        cx, cy = centre
        bounds = (cx - radius - 0.02, cy - radius - 0.02, cx + radius + 0.02, cy + radius + 0.02)
        self._for_each_sample(bounds, lambda x, y: math.hypot(x - cx, y - cy) <= radius, colour)

    def stroke_ring(self, centre, radius, width, colour):
        cx, cy = centre
        half = width / 2.0
        bounds = (cx - radius - half - 0.02, cy - radius - half - 0.02,
                  cx + radius + half + 0.02, cy + radius + half + 0.02)
        self._for_each_sample(bounds, lambda x, y: abs(math.hypot(x - cx, y - cy) - radius) <= half, colour)

    def write(self, path):
        raw = bytearray()
        for y in range(self.size):
            raw.append(0)
            for x in range(self.size):
                raw.extend(self.pixels[y * self.size + x])
        compressed = zlib.compress(bytes(raw), 9)

        def chunk(kind, payload):
            return (struct.pack('>I', len(payload)) + kind + payload
                    + struct.pack('>I', zlib.crc32(kind + payload) & 0xffffffff))

        header = struct.pack('>IIBBBBB', self.size, self.size, 8, 6, 0, 0, 0)
        path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', header)
                         + chunk(b'IDAT', compressed) + chunk(b'IEND', b''))


def render(size, samples=3):
    raster = Raster(size, samples)
    raster.fill_rounded_rect(BADGE_FILL + (255,))
    raster.stroke_rounded_rect(BADGE_EDGE + (255,))
    raster.stroke_polyline(wave_points(), WAVE_WIDTH, WAVE + (255,))
    raster.stroke_polyline(slope_points(), SLOPE_WIDTH, SLOPE + (255,), dot_radius=SLOPE_DOT_RADIUS)
    raster.stroke_ring(KNOB_CENTRE, KNOB_RADIUS, KNOB_RING_WIDTH, KNOB + (255,))
    raster.stroke_polyline([KNOB_CENTRE, KNOB_NOTCH], KNOB_NOTCH_WIDTH, KNOB + (255,))
    return raster


def svg():
    points = lambda seq: ' '.join(f'{x:.4f},{y:.4f}' for x, y in seq)
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1 1" width="1024" height="1024" role="img" aria-label="G-AcidBase logo">
  <title>G-AcidBase</title>
  <desc>Acid-green saw wave over a cyan resonant filter slope with a violet control knob, on a graphite badge.</desc>
  <rect x="{BADGE_INSET:.4f}" y="{BADGE_INSET:.4f}" width="{1 - 2 * BADGE_INSET:.4f}" height="{1 - 2 * BADGE_INSET:.4f}" rx="{BADGE_RADIUS:.4f}" fill="#10161f"/>
  <rect x="{BADGE_INSET:.4f}" y="{BADGE_INSET:.4f}" width="{1 - 2 * BADGE_INSET:.4f}" height="{1 - 2 * BADGE_INSET:.4f}" rx="{BADGE_RADIUS:.4f}" fill="none" stroke="#33425a" stroke-width="{BADGE_EDGE_WIDTH:.4f}"/>
  <polyline points="{points(wave_points())}" fill="none" stroke="#b8ff38" stroke-width="{WAVE_WIDTH:.4f}" stroke-linecap="round" stroke-linejoin="round"/>
  <polyline points="{points(slope_points())}" fill="none" stroke="#42e8da" stroke-width="{SLOPE_WIDTH:.4f}" stroke-linecap="round"/>
  <circle cx="{SLOPE_END[0]:.4f}" cy="{SLOPE_END[1]:.4f}" r="{SLOPE_DOT_RADIUS:.4f}" fill="#42e8da"/>
  <circle cx="{KNOB_CENTRE[0]:.4f}" cy="{KNOB_CENTRE[1]:.4f}" r="{KNOB_RADIUS:.4f}" fill="none" stroke="#b290ff" stroke-width="{KNOB_RING_WIDTH:.4f}"/>
  <polyline points="{points([KNOB_CENTRE, KNOB_NOTCH])}" fill="none" stroke="#b290ff" stroke-width="{KNOB_NOTCH_WIDTH:.4f}" stroke-linecap="round"/>
</svg>
'''


def main():
    ASSETS.mkdir(exist_ok=True)
    (ASSETS / 'G-AcidBase-Logo.svg').write_text(svg(), encoding='utf-8')
    written = []
    for size in (64, 128, 256, 512, 1024):
        samples = 4 if size <= 512 else 2
        target = ASSETS / f'G-AcidBase-Logo-{size}.png'
        render(size, samples).write(target)
        written.append(f'{target.name} ({target.stat().st_size:,} bytes)')
    print('Logo assets written:\n  ' + '\n  '.join(['G-AcidBase-Logo.svg'] + written))


if __name__ == '__main__':
    main()