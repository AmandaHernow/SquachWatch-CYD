// SquachWatches in the Wild -- the flasher's install counter.
//
// A Cloudflare Worker at count.squachwatch.com with one KV namespace bound
// as COUNTS. The flasher page sends one POST /hit when an install finishes,
// naming the board; GET / returns the running total and the per-board
// tally for the page to show.
//
// It keeps numbers and nothing else: no address, no browser, no time, no
// cookie. The board name is checked against the list below, so the tally
// only ever holds boards the flasher actually offers.
//
// The total starts at BASE, a guess at the installs before counting began
// (2026-10); the per-board tally starts at zero.

const BASE = 5000;
const BOARDS = new Set([
  "cyd", "cyd-fast", "cyd-ili9341", "cyd-ili9341-fast",
  "freenove32", "cyd32c", "cyd35-fast", "cyd35c-fast",
  "awok", "rlphantom-r",
  "freenove-s3", "freenove-s3-35", "crowpanel7", "tdeck-plus",
  "nm-cyd-c5", "marauder-v8",
  "sticks3", "cardputer-adv",
  "twatch-s3",
]);
const ORIGINS = new Set(["https://squachwatch.com", "https://www.squachwatch.com"]);

function cors(origin) {
  return {
    "Access-Control-Allow-Origin": ORIGINS.has(origin) ? origin : "https://squachwatch.com",
    "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
    "Access-Control-Allow-Headers": "Content-Type",
    "Vary": "Origin",
  };
}

async function tally(env) {
  const total = parseInt((await env.COUNTS.get("total")) || "0", 10);
  const raw = await env.COUNTS.get("boards");
  const boards = raw ? JSON.parse(raw) : {};
  return { total, boards };
}

function json(body, origin, extra) {
  return new Response(JSON.stringify(body), {
    headers: { "Content-Type": "application/json", ...cors(origin), ...(extra || {}) },
  });
}

export default {
  async fetch(request, env) {
    const origin = request.headers.get("Origin") || "";
    const url = new URL(request.url);

    if (request.method === "OPTIONS") return new Response(null, { headers: cors(origin) });

    if (request.method === "GET" && url.pathname === "/") {
      const t = await tally(env);
      return json({ total: BASE + t.total, boards: t.boards }, origin, { "Cache-Control": "public, max-age=60" });
    }

    if (request.method === "POST" && url.pathname === "/hit") {
      // Only the flasher page counts; a browser always sends its Origin on a POST.
      if (!ORIGINS.has(origin)) return new Response("no", { status: 403 });
      let board = "";
      try { board = String((await request.json()).board || ""); } catch (e) {}
      if (!BOARDS.has(board)) return new Response("unknown board", { status: 400, headers: cors(origin) });
      // KV has no atomic add; two installs landing in the same instant can
      // lose one. Fine for a number that is only for fun.
      const t = await tally(env);
      t.total += 1;
      t.boards[board] = (t.boards[board] || 0) + 1;
      await env.COUNTS.put("total", String(t.total));
      await env.COUNTS.put("boards", JSON.stringify(t.boards));
      return json({ total: BASE + t.total, boards: t.boards }, origin);
    }

    return new Response("SquachWatches in the Wild", { status: 404, headers: cors(origin) });
  },
};
