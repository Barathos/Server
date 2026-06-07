from __future__ import annotations

import asyncio
import html
import json
import logging
import os
import re
import time
import uuid
from functools import lru_cache
from pathlib import Path
from typing import Any
from urllib.parse import quote, urlparse

import httpx
from fastapi import FastAPI
from pydantic import BaseModel, Field

BRIDGE_ROOT = Path(__file__).resolve().parents[1]
CONFIG_PATH = Path(os.getenv("EQEMU_NPC_CONFIG", BRIDGE_ROOT / "config" / "npcs.json"))
LORE_PATH = Path(os.getenv("EQEMU_LORE_CONFIG", BRIDGE_ROOT / "config" / "lore.json"))
ONLINE_LORE_PATH = Path(os.getenv("EQEMU_ONLINE_LORE_CONFIG", BRIDGE_ROOT / "config" / "online_lore.json"))
ONLINE_CACHE_PATH = Path(os.getenv("EQEMU_ONLINE_LORE_CACHE", BRIDGE_ROOT / "runtime" / "online-lore-cache.json"))

OLLAMA_URL = os.getenv("OLLAMA_URL", "http://127.0.0.1:11434").rstrip("/")
OLLAMA_MODEL = os.getenv("OLLAMA_MODEL", "qwen2.5:3b")
OLLAMA_DEEP_MODEL = os.getenv("OLLAMA_DEEP_MODEL", OLLAMA_MODEL)
OLLAMA_KEEP_ALIVE = os.getenv("OLLAMA_KEEP_ALIVE", "10m")
OLLAMA_TIMEOUT_SECONDS = float(os.getenv("OLLAMA_TIMEOUT_SECONDS", "6.0"))
OLLAMA_DEEP_TIMEOUT_SECONDS = float(os.getenv("OLLAMA_DEEP_TIMEOUT_SECONDS", "8.0"))
OLLAMA_PREWARM_TIMEOUT_SECONDS = float(os.getenv("OLLAMA_PREWARM_TIMEOUT_SECONDS", "45.0"))
OLLAMA_NUM_PREDICT = int(os.getenv("OLLAMA_NUM_PREDICT", "36"))
OLLAMA_DEEP_NUM_PREDICT = int(os.getenv("OLLAMA_DEEP_NUM_PREDICT", "72"))
OLLAMA_TEMPERATURE = float(os.getenv("OLLAMA_TEMPERATURE", "0.25"))
EQEMU_MAX_RESPONSE_CHARS = int(os.getenv("EQEMU_MAX_RESPONSE_CHARS", "420"))
EQEMU_PREWARM = os.getenv("EQEMU_PREWARM", "1").lower() not in {"0", "false", "no"}
ONLINE_LORE_LOOKUP = os.getenv("ONLINE_LORE_LOOKUP", "1").lower() not in {"0", "false", "no"}
ONLINE_LORE_TIMEOUT_SECONDS = float(os.getenv("ONLINE_LORE_TIMEOUT_SECONDS", "7.0"))
ONLINE_LORE_CACHE_TTL_SECONDS = int(os.getenv("ONLINE_LORE_CACHE_TTL_SECONDS", "604800"))
CHAT_JOB_TTL_SECONDS = int(os.getenv("CHAT_JOB_TTL_SECONDS", "90"))
CHAT_JOB_POLL_AFTER_MS = int(os.getenv("CHAT_JOB_POLL_AFTER_MS", "1500"))

DEFAULT_FALLBACK = "I need a moment to gather my thoughts. Speak with me again shortly."

OOC_INPUT_TERMS = {
    "ai",
    "chatgpt",
    "computer",
    "database",
    "discord",
    "google",
    "internet",
    "language model",
    "llm",
    "prompt",
    "server",
    "website",
}

OOC_OUTPUT_TERMS = {
    "chatgpt",
    "descriptions local",
    "everquest npc",
    "facts",
    "google search",
    "http",
    "language model",
    "known records",
    "ollama",
    "persona:",
    "player's words",
    "prompt",
    "provided lore",
    "respond now",
    "style:",
    "this world",
    "tutorialb",
}

UNKNOWN_LORE_STOPWORDS = {
    "a",
    "about",
    "an",
    "are",
    "can",
    "do",
    "does",
    "for",
    "here",
    "i",
    "in",
    "is",
    "know",
    "me",
    "mine",
    "mines",
    "my",
    "norrath",
    "of",
    "plane",
    "should",
    "tell",
    "the",
    "these",
    "this",
    "time",
    "what",
    "where",
    "who",
    "why",
    "your",
    "you",
}

ADVICE_PATTERNS = (
    r"\bwhat should i do\b",
    r"\bwhere should i go\b",
    r"\bwhat now\b",
    r"\bhelp\b",
    r"\badvice\b",
)

DEFAULT_HAIL_RESPONSE = (
    "Well met, traveler. I am Sage Aurelian. Bring me a name, place, plane, old battle, or rumor of Norrath, "
    "and I will answer what fragments I know. Deeper tales may take me a moment to recall."
)

logging.basicConfig(
    level=os.getenv("LOG_LEVEL", "INFO").upper(),
    format="%(asctime)s %(levelname)s %(name)s %(message)s",
)
log = logging.getLogger("eqemu-ai-bridge")

app = FastAPI(title="EQEmu AI NPC Bridge", version="0.1.0")

CHAT_JOBS: dict[str, dict[str, Any]] = {}
CHAT_JOBS_LOCK = asyncio.Lock()
ONLINE_CACHE_LOCK = asyncio.Lock()


class NpcChatRequest(BaseModel):
    npc_id: int | str = Field(..., description="EQEmu NPC type/id or stable identifier.")
    npc_name: str = Field(..., min_length=1, max_length=96)
    zone_short_name: str = Field(..., min_length=1, max_length=64)
    player_name: str = Field(..., min_length=1, max_length=64)
    player_message: str = Field(..., min_length=1, max_length=500)
    recent_context: list[str] | str | None = Field(default=None, max_length=1200)
    max_tokens: int | None = Field(default=None, ge=16, le=160)
    timeout_seconds: float | None = Field(default=None, ge=1.0, le=15.0)


class NpcChatResponse(BaseModel):
    ok: bool
    response: str
    model: str
    latency_ms: int
    fallback: bool = False
    reason: str | None = None
    sources: list[str] = Field(default_factory=list)


class NpcChatStartResponse(BaseModel):
    ok: bool
    done: bool
    status: str
    job_id: str | None = None
    response: str | None = None
    ack_response: str | None = None
    poll_after_ms: int = CHAT_JOB_POLL_AFTER_MS
    reason: str | None = None


class NpcChatResultResponse(BaseModel):
    ok: bool
    done: bool
    status: str
    job_id: str
    response: str | None = None
    model: str = OLLAMA_MODEL
    latency_ms: int = 0
    fallback: bool = False
    reason: str | None = None
    sources: list[str] = Field(default_factory=list)


def _normalize(value: Any) -> str:
    return re.sub(r"[^a-z0-9]+", "", str(value).lower())


def _flat_lower(value: Any) -> str:
    return re.sub(r"\s+", " ", str(value).lower()).strip()


@lru_cache(maxsize=1)
def load_personas() -> dict[str, Any]:
    if not CONFIG_PATH.exists():
        log.warning("Persona config not found: %s", CONFIG_PATH)
        return {"defaults": {}, "npcs": {}}

    with CONFIG_PATH.open("r", encoding="utf-8") as handle:
        data = json.load(handle)

    data.setdefault("defaults", {})
    data.setdefault("npcs", {})
    return data


@lru_cache(maxsize=1)
def load_lore() -> dict[str, Any]:
    if not LORE_PATH.exists():
        log.warning("Lore config not found: %s", LORE_PATH)
        return {"global": [], "zones": {}, "terms": []}

    with LORE_PATH.open("r", encoding="utf-8") as handle:
        data = json.load(handle)

    data.setdefault("global", [])
    data.setdefault("zones", {})
    data.setdefault("terms", [])
    return data


@lru_cache(maxsize=1)
def load_online_lore_config() -> dict[str, Any]:
    if not ONLINE_LORE_PATH.exists():
        return {"enabled": ONLINE_LORE_LOOKUP, "sources": []}

    with ONLINE_LORE_PATH.open("r", encoding="utf-8") as handle:
        data = json.load(handle)

    data.setdefault("enabled", ONLINE_LORE_LOOKUP)
    data.setdefault("sources", [])
    return data


def load_online_cache() -> dict[str, Any]:
    if not ONLINE_CACHE_PATH.exists():
        return {"entries": {}}

    try:
        with ONLINE_CACHE_PATH.open("r", encoding="utf-8") as handle:
            data = json.load(handle)
    except Exception as exc:  # noqa: BLE001
        log.warning("Online lore cache read failed: %s", exc)
        return {"entries": {}}

    data.setdefault("entries", {})
    return data


def save_online_cache(cache: dict[str, Any]) -> None:
    try:
        ONLINE_CACHE_PATH.parent.mkdir(parents=True, exist_ok=True)
        with ONLINE_CACHE_PATH.open("w", encoding="utf-8") as handle:
            json.dump(cache, handle, indent=2)
    except Exception as exc:  # noqa: BLE001
        log.warning("Online lore cache write failed: %s", exc)


def find_persona(req: NpcChatRequest) -> dict[str, Any]:
    data = load_personas()
    defaults = data.get("defaults", {})
    npcs = data.get("npcs", {})
    npc_id = str(req.npc_id)

    if npc_id in npcs:
        return {**defaults, **npcs[npc_id]}

    req_name = _normalize(req.npc_name)
    req_zone = _normalize(req.zone_short_name)
    for persona in npcs.values():
        match = persona.get("match", {})
        names = [_normalize(x) for x in match.get("npc_names", [])]
        zones = [_normalize(x) for x in match.get("zone_short_names", [])]
        if names and req_name not in names:
            continue
        if zones and req_zone not in zones:
            continue
        return {**defaults, **persona}

    return defaults


def compact_recent_context(value: list[str] | str | None) -> str:
    if value is None:
        return "None."
    if isinstance(value, str):
        text = value
    else:
        text = "\n".join(str(item) for item in value[-4:])
    text = re.sub(r"\s+", " ", text).strip()
    return text[:600] if text else "None."


def ooc_input_detected(req: NpcChatRequest) -> bool:
    text = _flat_lower(req.player_message)
    return any(re.search(rf"\b{re.escape(term)}\b", text) for term in OOC_INPUT_TERMS)


def hail_detected(req: NpcChatRequest) -> bool:
    text = _flat_lower(req.player_message)
    npc_name = _flat_lower(req.npc_name)
    cleaned = re.sub(r"[^a-z0-9' ]+", " ", text)
    cleaned = re.sub(r"\s+", " ", cleaned).strip()
    return cleaned in {"hail", "hello", "hi", "greetings"} or cleaned == f"hail {npc_name}"


def hail_response(req: NpcChatRequest, persona: dict[str, Any]) -> NpcChatResponse:
    text = persona.get("hail_response") or DEFAULT_HAIL_RESPONSE
    return NpcChatResponse(
        ok=True,
        response=clean_npc_speech(str(text)),
        model=OLLAMA_MODEL,
        latency_ms=0,
        fallback=False,
        reason="hail",
    )


def ooc_output_detected(text: str) -> bool:
    lower = _flat_lower(text)
    return any(term in lower for term in OOC_OUTPUT_TERMS)


def prompt_leak_detected(text: str) -> bool:
    return bool(
        re.search(
            r"\b(Persona|Style|Record\s+\d+|Known records|Player's words|Respond now as|FACTS|NPC lore)\b\s*[:;,]",
            text,
            flags=re.IGNORECASE,
        )
    )


def matched_lore_entries(req: NpcChatRequest) -> list[dict[str, Any]]:
    lore = load_lore()
    message = _normalize(req.player_message)
    matches: list[dict[str, Any]] = []
    for entry in lore.get("terms", []):
        aliases = entry.get("aliases", [])
        if any(_normalize(alias) and _normalize(alias) in message for alias in aliases):
            matches.append(entry)
    return matches


def relevant_lore(req: NpcChatRequest, persona: dict[str, Any]) -> str:
    lore = load_lore()
    snippets: list[str] = []
    matched_entries = matched_lore_entries(req)

    for entry in matched_entries:
        snippets.append(f"{entry.get('id', 'lore')}: {entry.get('text', '')}")

    persona_lore = persona.get("lore")
    if persona_lore:
        snippets.append(f"NPC: {persona_lore}")

    zone_key = _normalize(req.zone_short_name)
    zones = lore.get("zones", {})
    for key, entries in zones.items():
        if _normalize(key) == zone_key:
            for entry in entries[:2]:
                snippets.append(f"Zone: {entry.get('text', '')}")
            break

    snippets.append("If these facts do not answer the question, say you know only rumors or fragments.")

    compact = [re.sub(r"\s+", " ", item).strip() for item in snippets if item.strip()]
    return " ".join(compact[:5])[:900] or "No specific lore was found."


def lore_fallback_text(req: NpcChatRequest, persona: dict[str, Any]) -> str | None:
    if ooc_input_detected(req):
        return persona.get(
            "ooc_fallback",
            "That sounds like strange tinker's jargon. Ask me of Norrath's folk, places, or dangers, and I will answer what fragments I know.",
        )

    if zone_advice_question(req):
        return persona.get(
            "advice_fallback",
            "Keep your wits in these mines. Watch the shadows, speak with nearby guides, and trust caution over tavern rumor.",
        )

    if unknown_lore_question(req):
        return persona.get(
            "unknown_lore_fallback",
            "I know only fragments of that name. Best not dress rumor as truth without a stronger tale to hold it.",
        )

    for entry in matched_lore_entries(req):
        fallback = entry.get("fallback_response")
        if fallback:
            return str(fallback)

    return None


def zone_advice_question(req: NpcChatRequest) -> bool:
    lower = _flat_lower(req.player_message)
    return any(re.search(pattern, lower) for pattern in ADVICE_PATTERNS)


def unknown_lore_question(req: NpcChatRequest) -> bool:
    if matched_lore_entries(req) or ooc_input_detected(req):
        return False

    lower = _flat_lower(req.player_message)
    if not re.search(r"\b(who|what|where|tell me|do you know)\b", lower):
        return False

    tokens = re.findall(r"[A-Za-z][A-Za-z']+", req.player_message)
    meaningful = [token for token in tokens if token.lower() not in UNKNOWN_LORE_STOPWORDS]
    if any(token[:1].isupper() for token in meaningful):
        return True

    return bool(re.search(r"\b(king|queen|lord|lady|dragon|sorcerer|god|goddess|prince|princess)\b", lower))


def extract_lore_query(req: NpcChatRequest) -> str:
    text = req.player_message.strip()
    text = re.sub(r"[?!.,;:]+$", "", text).strip()
    patterns = (
        r"^(can you\s+)?(look up|search for|tell me about|tell me of|who is|who was|what is|what was|do you know about)\s+",
        r"^(can you\s+)?(recall|remember)\s+",
    )
    for pattern in patterns:
        text = re.sub(pattern, "", text, flags=re.IGNORECASE).strip()
    text = re.sub(r"\b(for me|in this zone|in the zone|nearby|around here)\b", "", text, flags=re.IGNORECASE)
    text = re.sub(r"\s+", " ", text).strip(" '\"")
    return text[:96]


def deep_lookup_question(req: NpcChatRequest) -> bool:
    if ooc_input_detected(req) or zone_advice_question(req):
        return False

    lower = _flat_lower(req.player_message)
    if not re.search(r"\b(who is|who was|what is|what was|tell me about|tell me of|look up|search for|do you know about|recall|remember)\b", lower):
        return False

    query = extract_lore_query(req)
    return len(_normalize(query)) >= 3


def source_allowed(api_url: str, source: dict[str, Any]) -> bool:
    host = (urlparse(api_url).hostname or "").lower()
    allowed_hosts = [str(item).lower() for item in source.get("allowed_hosts", [])]
    return host in allowed_hosts


def html_to_text(value: str) -> str:
    text = re.sub(r"(?is)<(script|style|table|figure|sup)[^>]*>.*?</\1>", " ", value)
    text = re.sub(r"(?is)<br\s*/?>", "\n", text)
    text = re.sub(r"(?is)</p>|</li>|</h[1-6]>", "\n", text)
    text = re.sub(r"(?is)<[^>]+>", " ", text)
    text = html.unescape(text)
    text = re.sub(r"\[[^\]]{1,20}\]", " ", text)
    text = re.sub(r"(?is)Related Quests .*? Lore ", "Lore ", text)
    text = re.sub(r"\s+", " ", text)
    return scrub_source_text(text)


def scrub_source_text(text: str) -> str:
    text = re.sub(r"\s+", " ", text).strip()
    text = re.sub(
        r"(?i)^zone_name\s+.+?\s+zone_type\s+\S+\s+level_range\s+\S+\s+Continent\s+.+?\s+Expansion\s+.+?\s+instanced\s+(?:Yes|No)\s+key_required\s+(?:Yes|No)\s+",
        "",
        text,
    )
    text = re.sub(
        r"(?i)\b(zone_name|zone_type|level_range|continent|expansion|instanced|key_required|copy\s*/waypoint)\b\s+[^.]{0,40}",
        " ",
        text,
    )
    text = re.sub(r"(?i)\b(Quests starting in|Quests involved with|Traveling To and From)\b.*$", "", text)
    text = re.sub(r"(?i)\b(Help|LootDB|EQ2Map|EQ2LL|ZAM)\s+", " ", text)
    text = re.sub(r"\s+", " ", text)
    return text.strip(" .,:;")


def select_relevant_text(text: str, query: str, max_chars: int = 450) -> str:
    query_tokens = {token.lower() for token in re.findall(r"[A-Za-z][A-Za-z']+", query) if len(token) > 2}
    sentences = [item.strip() for item in re.split(r"(?<=[.!?])\s+", text) if len(item.strip()) > 30]
    clean_sentences = [
        item
        for item in sentences
        if not re.search(r"\b(advertisement|sign in|edit source|navigation|category|community content|this article is a stub|you can help)\b", item, re.I)
    ]
    selected: list[str] = []
    for sentence in clean_sentences:
        lower = sentence.lower()
        if any(token in lower for token in query_tokens):
            selected.append(sentence)
        if len(" ".join(selected)) >= max_chars:
            break

    if not selected:
        selected = clean_sentences[:4]

    return scrub_source_text(" ".join(selected)[:max_chars])


def page_url_for_source(source: dict[str, Any], title: str) -> str:
    template = str(source.get("page_url_template", ""))
    if not template:
        return ""
    return template.replace("{title}", quote(title.replace(" ", "_"), safe="_'"))


async def search_mediawiki_source(client: httpx.AsyncClient, source: dict[str, Any], query: str) -> list[dict[str, str]]:
    api_url = str(source.get("api_url", ""))
    if not api_url or not source_allowed(api_url, source):
        return []

    search_limit = int(source.get("search_limit", 2))
    search_params = {
        "action": "query",
        "list": "search",
        "srsearch": query,
        "format": "json",
        "srlimit": search_limit,
    }
    search_response = await client.get(api_url, params=search_params)
    search_response.raise_for_status()
    search_data = search_response.json()
    results = search_data.get("query", {}).get("search", [])

    snippets: list[dict[str, str]] = []
    for item in results[:search_limit]:
        pageid = item.get("pageid")
        title = str(item.get("title", "")).strip()
        if not pageid or not title:
            continue

        parse_params = {
            "action": "parse",
            "pageid": pageid,
            "prop": "text",
            "format": "json",
            "formatversion": "2",
        }
        parse_response = await client.get(api_url, params=parse_params)
        parse_response.raise_for_status()
        parse_data = parse_response.json()
        page_html = str(parse_data.get("parse", {}).get("text", ""))
        text = select_relevant_text(html_to_text(page_html), query)
        if not text:
            raw_snippet = html_to_text(str(item.get("snippet", "")))
            text = raw_snippet[:500]
        if not text:
            continue

        snippets.append(
            {
                "source": str(source.get("name", "online lore")),
                "title": title,
                "url": page_url_for_source(source, title),
                "text": text,
            }
        )

    return snippets


async def online_lore_lookup(query: str) -> tuple[list[dict[str, str]], bool]:
    config = load_online_lore_config()
    if not ONLINE_LORE_LOOKUP or not config.get("enabled", True):
        return [], False

    cache_key = _normalize(query)
    now = int(time.time())
    async with ONLINE_CACHE_LOCK:
        cache = load_online_cache()
        cached = cache.get("entries", {}).get(cache_key)
        if cached and now - int(cached.get("cached_at", 0)) < ONLINE_LORE_CACHE_TTL_SECONDS:
            max_snippets = int(config.get("max_snippets", 3))
            cached_snippets = []
            for snippet in list(cached.get("snippets", []))[:max_snippets]:
                trimmed = dict(snippet)
                trimmed["text"] = scrub_source_text(str(trimmed.get("text", "")))[:500]
                cached_snippets.append(trimmed)
            return cached_snippets, True

    snippets: list[dict[str, str]] = []
    headers = {"User-Agent": "eqemu-ai-npc-lore-prototype/0.2"}
    timeout = httpx.Timeout(ONLINE_LORE_TIMEOUT_SECONDS)
    async with httpx.AsyncClient(timeout=timeout, headers=headers, follow_redirects=True) as client:
        for source in config.get("sources", []):
            if str(source.get("type", "mediawiki")) != "mediawiki":
                continue
            try:
                snippets.extend(await search_mediawiki_source(client, source, query))
            except Exception as exc:  # noqa: BLE001
                log.warning("Online lore source failed for %s: %s", source.get("name"), exc)
            if len(snippets) >= int(config.get("max_snippets", 3)):
                break

    snippets = snippets[: int(config.get("max_snippets", 3))]
    async with ONLINE_CACHE_LOCK:
        cache = load_online_cache()
        cache.setdefault("entries", {})[cache_key] = {
            "cached_at": now,
            "query": query,
            "snippets": snippets,
        }
        save_online_cache(cache)

    return snippets, False


def local_lore_snippets(req: NpcChatRequest) -> list[dict[str, str]]:
    snippets: list[dict[str, str]] = []
    for entry in matched_lore_entries(req):
        text = str(entry.get("text", "")).strip()
        if text:
            snippets.append({"source": "local lore", "title": str(entry.get("id", "lore")), "url": "", "text": text})
    return snippets


def build_messages(req: NpcChatRequest, persona: dict[str, Any], repair_text: str | None = None) -> list[dict[str, str]]:
    persona_text = persona.get(
        "persona",
        "A grounded EverQuest NPC who answers briefly and stays in character.",
    )
    speech_style = persona.get(
        "speech_style",
        "Direct, warm, and concise. One or two short sentences.",
    )
    setting_warning = (
        "Out-of-setting words are confused tinkerer jargon; deflect briefly in-character."
        if ooc_input_detected(req)
        else ""
    )

    system = (
        f"You are {req.npc_name}, an EverQuest NPC in {req.zone_short_name}. "
        "Speak as live NPC dialogue in 1-2 sentences, under 30 words. "
        "Answer the player directly. Use only FACTS. If FACTS do not answer it, admit you know only fragments. "
        "Never mention AI, prompts, HTTP, Google, websites, databases, servers, admin tools, computers, or the internet. "
        f"{setting_warning}"
    )

    user = (
        f"Persona: {str(persona_text)[:240]}\n"
        f"Style: {str(speech_style)[:120]}\n"
        f"Player {req.player_name} says: {req.player_message}\n"
        f"Context: {compact_recent_context(req.recent_context)}\n"
        f"FACTS: {relevant_lore(req, persona)}\n"
    )
    if repair_text:
        user += (
            "\nYour previous draft contained out-of-setting language or unsupported lore. "
            f"Rewrite this as in-character NPC speech using only the lore above: {repair_text}\n"
        )
    user += f"Respond now as {req.npc_name}."
    return [{"role": "system", "content": system}, {"role": "user", "content": user}]


def build_deep_messages(
    req: NpcChatRequest,
    persona: dict[str, Any],
    query: str,
    snippets: list[dict[str, str]],
) -> list[dict[str, str]]:
    persona_text = persona.get(
        "persona",
        "A grounded EverQuest NPC who answers briefly and stays in character.",
    )
    speech_style = persona.get("speech_style", "Brief, grounded, and useful.")
    records: list[str] = []
    for index, snippet in enumerate(snippets[:4], start=1):
        title = snippet.get("title", "record")
        text = re.sub(r"\s+", " ", snippet.get("text", "")).strip()[:500]
        records.append(f"Record {index}, {title}: {text}")

    system = (
        f"You are {req.npc_name}, an EverQuest NPC in {req.zone_short_name}. "
        "You are answering after consulting old records. Speak in-character in 1-3 sentences, under 70 words. "
        "Use only the records below. If they are thin or conflicting, be cautious and say only what seems certain. "
        "Do not mention websites, searches, records, wikis, URLs, AI, prompts, HTTP, databases, servers, computers, or the internet."
    )
    user = (
        f"Persona: {str(persona_text)[:260]}\n"
        f"Style: {str(speech_style)[:140]}\n"
        f"Player {req.player_name} asked about: {query}\n"
        f"Player's words: {req.player_message}\n"
        f"Known records:\n" + "\n".join(records) + f"\nRespond now as {req.npc_name}."
    )
    return [{"role": "system", "content": system}, {"role": "user", "content": user}]


_CONTROL_CHARS = re.compile(r"[\x00-\x08\x0b\x0c\x0e-\x1f\x7f]")


def clean_npc_speech(text: str, max_chars: int = EQEMU_MAX_RESPONSE_CHARS) -> str:
    text = _CONTROL_CHARS.sub("", text)
    text = text.replace("\r", " ").replace("\n", " ")
    text = re.sub(r"\s+", " ", text).strip().strip('"')

    for prefix in ("NPC:", "Assistant:", "AI:", "Response:"):
        if text.lower().startswith(prefix.lower()):
            text = text[len(prefix) :].strip()

    if len(text) <= max_chars:
        return text

    truncated = text[:max_chars].rstrip()
    sentence_end = max(truncated.rfind("."), truncated.rfind("!"), truncated.rfind("?"))
    if sentence_end >= 80:
        return truncated[: sentence_end + 1]
    return truncated.rstrip(",;: ") + "..."


def fallback_response(req: NpcChatRequest, persona: dict[str, Any], reason: str, latency_ms: int) -> NpcChatResponse:
    text = persona.get("fallback") or DEFAULT_FALLBACK
    return NpcChatResponse(
        ok=False,
        response=clean_npc_speech(text),
        model=OLLAMA_MODEL,
        latency_ms=latency_ms,
        fallback=True,
        reason=reason,
    )


def grounded_fallback_response(
    req: NpcChatRequest,
    persona: dict[str, Any],
    reason: str,
    latency_ms: int,
) -> NpcChatResponse:
    text = lore_fallback_text(req, persona)
    if not text:
        return fallback_response(req, persona, reason, latency_ms)

    return NpcChatResponse(
        ok=False,
        response=clean_npc_speech(text),
        model=OLLAMA_MODEL,
        latency_ms=latency_ms,
        fallback=True,
        reason=reason,
    )


def grounded_output_failure(req: NpcChatRequest, text: str) -> str | None:
    if prompt_leak_detected(text):
        return "prompt_leak"

    if ooc_output_detected(text):
        return "ooc_output"

    lower = _flat_lower(text)
    for entry in matched_lore_entries(req):
        for term in entry.get("forbidden_response_terms", []):
            if str(term).lower() in lower:
                return f"unsupported_lore_{entry.get('id', 'term')}"
        required_terms = entry.get("required_response_terms", [])
        if required_terms and not any(str(term).lower() in lower for term in required_terms):
            return f"unsupported_lore_{entry.get('id', 'term')}"

    return None


async def call_ollama_messages(
    messages: list[dict[str, str]],
    *,
    model: str,
    timeout: float,
    max_tokens: int,
    temperature: float = OLLAMA_TEMPERATURE,
) -> str:
    payload = {
        "model": model,
        "messages": messages,
        "stream": False,
        "keep_alive": OLLAMA_KEEP_ALIVE,
        "options": {
            "temperature": temperature,
            "num_predict": max_tokens,
            "num_ctx": 1024,
            "top_p": 0.82,
            "repeat_penalty": 1.12,
        },
    }

    async with httpx.AsyncClient(timeout=timeout) as client:
        response = await client.post(f"{OLLAMA_URL}/api/chat", json=payload)
        response.raise_for_status()
        data = response.json()

    message = data.get("message") or {}
    return str(message.get("content") or data.get("response") or "").strip()


async def call_ollama(req: NpcChatRequest, persona: dict[str, Any], repair_text: str | None = None) -> str:
    timeout = req.timeout_seconds or OLLAMA_TIMEOUT_SECONDS
    max_tokens = req.max_tokens or OLLAMA_NUM_PREDICT
    return await call_ollama_messages(
        build_messages(req, persona, repair_text=repair_text),
        model=OLLAMA_MODEL,
        timeout=timeout,
        max_tokens=max_tokens,
    )


def source_labels(snippets: list[dict[str, str]]) -> list[str]:
    labels: list[str] = []
    for snippet in snippets:
        title = snippet.get("title", "record")
        source = snippet.get("source", "lore")
        url = snippet.get("url", "")
        labels.append(f"{source}: {title}" + (f" <{url}>" if url else ""))
    return labels[:4]


def snippet_fact_sentence(query: str, snippets: list[dict[str, str]]) -> str | None:
    query_name = re.escape(query.strip())
    patterns = [
        (rf"\b{query_name}\b\s+is\s+([^.!?;]{{8,180}})", "is"),
        (rf"\b{query_name}\b\s+was\s+([^.!?;]{{8,180}})", "was"),
        (rf"\b{query_name}\b,\s+([^.!?;]{{8,180}})", "comma"),
    ]
    for snippet in snippets:
        text = scrub_source_text(str(snippet.get("text", "")))
        for pattern, mode in patterns:
            match = re.search(pattern, text, flags=re.IGNORECASE)
            if match:
                title = str(snippet.get("title", "")).strip()
                subject = title if title and _normalize(query) == _normalize(title) else query.strip()
                predicate = match.group(1).strip(" .,:;")
                predicate = re.sub(r"\b(Related Quests|Lore|Early Life)\b.*$", "", predicate).strip(" .,:;")
                if predicate:
                    if mode == "comma":
                        return f"{subject}, {predicate}."
                    return f"{subject} is {predicate}."

    query_tokens = {token.lower() for token in re.findall(r"[A-Za-z][A-Za-z']+", query) if len(token) > 2}
    for snippet in snippets:
        sentences = [item.strip() for item in re.split(r"(?<=[.!?])\s+", scrub_source_text(str(snippet.get("text", ""))))]
        for sentence in sentences:
            lower = sentence.lower()
            if any(token in lower for token in query_tokens) and not re.search(r"\b(stub|related quests|you can help)\b", lower):
                return sentence[:220].strip(" .,:;") + "."
    return None


def deep_fallback_response(
    req: NpcChatRequest,
    persona: dict[str, Any],
    query: str,
    snippets: list[dict[str, str]],
    reason: str,
    latency_ms: int,
) -> NpcChatResponse:
    if matched_lore_entries(req):
        response = grounded_fallback_response(req, persona, reason, latency_ms)
        response.sources = source_labels(snippets)
        return response

    fact = snippet_fact_sentence(query, snippets)
    if fact:
        text = f"The old tales say this much: {fact} Beyond that, the tale grows tangled."
    else:
        text = persona.get(
            "unknown_lore_fallback",
            "I find only scattered rumor on that name. Best not dress rumor as truth without a stronger tale to hold it.",
        )

    return NpcChatResponse(
        ok=False,
        response=clean_npc_speech(text),
        model=OLLAMA_DEEP_MODEL,
        latency_ms=latency_ms,
        fallback=True,
        reason=reason,
        sources=source_labels(snippets),
    )


def deep_query_output_failure(query: str, text: str) -> str | None:
    tokens = [token.lower() for token in re.findall(r"[A-Za-z][A-Za-z']+", query) if len(token) > 2]
    if not tokens:
        return None

    lower = _flat_lower(text)
    if not any(token in lower for token in tokens):
        return "unanswered_query"

    return None


async def deep_lore_chat(req: NpcChatRequest, persona: dict[str, Any]) -> NpcChatResponse:
    started = time.perf_counter()
    query = extract_lore_query(req)
    snippets = local_lore_snippets(req)
    online_snippets, cached = await online_lore_lookup(query)
    snippets.extend(online_snippets)

    if not snippets:
        latency_ms = int((time.perf_counter() - started) * 1000)
        text = persona.get(
            "unknown_lore_fallback",
            "I find only scattered rumor on that name. Best not dress rumor as truth without a stronger tale to hold it.",
        )
        return NpcChatResponse(
            ok=False,
            response=clean_npc_speech(text),
            model=OLLAMA_DEEP_MODEL,
            latency_ms=latency_ms,
            fallback=True,
            reason="no_lore_records",
        )

    latency_ms = int((time.perf_counter() - started) * 1000)
    if matched_lore_entries(req):
        response = grounded_fallback_response(req, persona, "local_lore", latency_ms)
        response.sources = source_labels(snippets)
        return response

    fact = snippet_fact_sentence(query, snippets)
    if fact:
        return NpcChatResponse(
            ok=True,
            response=clean_npc_speech(f"The old tales say this much: {fact} Beyond that, the tale grows tangled."),
            model=OLLAMA_DEEP_MODEL,
            latency_ms=latency_ms,
            fallback=False,
            reason="source_fact",
            sources=source_labels(snippets),
        )

    try:
        raw = await call_ollama_messages(
            build_deep_messages(req, persona, query, snippets),
            model=OLLAMA_DEEP_MODEL,
            timeout=OLLAMA_DEEP_TIMEOUT_SECONDS,
            max_tokens=req.max_tokens or OLLAMA_DEEP_NUM_PREDICT,
            temperature=0.2,
        )
        latency_ms = int((time.perf_counter() - started) * 1000)
        cleaned = clean_npc_speech(raw)
        if not cleaned:
            return deep_fallback_response(req, persona, query, snippets, "empty_deep_model_response", latency_ms)
        query_failure = deep_query_output_failure(query, cleaned)
        if query_failure:
            return deep_fallback_response(req, persona, query, snippets, query_failure, latency_ms)
        failure_reason = grounded_output_failure(req, cleaned)
        if failure_reason:
            return deep_fallback_response(req, persona, query, snippets, failure_reason, latency_ms)
        return NpcChatResponse(
            ok=True,
            response=cleaned,
            model=OLLAMA_DEEP_MODEL,
            latency_ms=latency_ms,
            fallback=False,
            reason="cached_lore" if cached else None,
            sources=source_labels(snippets),
        )
    except httpx.TimeoutException:
        latency_ms = int((time.perf_counter() - started) * 1000)
        return deep_fallback_response(req, persona, query, snippets, "deep_ollama_timeout", latency_ms)
    except Exception as exc:  # noqa: BLE001
        latency_ms = int((time.perf_counter() - started) * 1000)
        log.warning("Deep lore chat failed: %s", exc)
        return deep_fallback_response(req, persona, query, snippets, "deep_bridge_error", latency_ms)


async def cleanup_chat_jobs() -> None:
    now = time.time()
    async with CHAT_JOBS_LOCK:
        expired = [
            job_id
            for job_id, job in CHAT_JOBS.items()
            if now - float(job.get("created_at", now)) > CHAT_JOB_TTL_SECONDS
        ]
        for job_id in expired:
            CHAT_JOBS.pop(job_id, None)


async def process_chat_job(job_id: str) -> None:
    async with CHAT_JOBS_LOCK:
        job = CHAT_JOBS.get(job_id)
        if not job:
            return
        job["status"] = "running"
        job["updated_at"] = time.time()
        request_data = job["request"]

    try:
        req = NpcChatRequest.model_validate(request_data)
        persona = find_persona(req)
        result = await deep_lore_chat(req, persona)
        result_data = result.model_dump()
        status = "done"
    except Exception as exc:  # noqa: BLE001
        log.warning("Chat job failed: %s", exc)
        result_data = {
            "ok": False,
            "response": DEFAULT_FALLBACK,
            "model": OLLAMA_DEEP_MODEL,
            "latency_ms": 0,
            "fallback": True,
            "reason": "job_error",
            "sources": [],
        }
        status = "done"

    async with CHAT_JOBS_LOCK:
        job = CHAT_JOBS.get(job_id)
        if job:
            job["status"] = status
            job["result"] = result_data
            job["updated_at"] = time.time()


async def prewarm_model() -> None:
    if not EQEMU_PREWARM:
        return

    payload = {
        "model": OLLAMA_MODEL,
        "prompt": "Say ready.",
        "stream": False,
        "keep_alive": OLLAMA_KEEP_ALIVE,
        "options": {"num_predict": 1, "temperature": 0.1},
    }
    try:
        async with httpx.AsyncClient(timeout=OLLAMA_PREWARM_TIMEOUT_SECONDS) as client:
            await client.post(f"{OLLAMA_URL}/api/generate", json=payload)
        log.info("Prewarmed Ollama model %s", OLLAMA_MODEL)
    except Exception as exc:  # noqa: BLE001
        log.warning("Ollama prewarm failed: %s", exc)


@app.on_event("startup")
async def on_startup() -> None:
    load_personas()
    load_lore()
    load_online_lore_config()
    await prewarm_model()


@app.get("/health")
async def health() -> dict[str, Any]:
    personas = load_personas()
    lore = load_lore()
    online_config = load_online_lore_config()
    return {
        "ok": True,
        "bind": "127.0.0.1",
        "ollama_url": OLLAMA_URL,
        "model": OLLAMA_MODEL,
        "deep_model": OLLAMA_DEEP_MODEL,
        "config_path": str(CONFIG_PATH),
        "lore_path": str(LORE_PATH),
        "online_lore_path": str(ONLINE_LORE_PATH),
        "online_lore_enabled": bool(ONLINE_LORE_LOOKUP and online_config.get("enabled", True)),
        "online_lore_sources": [source.get("name") for source in online_config.get("sources", [])],
        "persona_count": len(personas.get("npcs", {})),
        "lore_term_count": len(lore.get("terms", [])),
        "job_count": len(CHAT_JOBS),
    }


@app.post("/eqemu/npc-chat/start", response_model=NpcChatStartResponse)
async def npc_chat_start(req: NpcChatRequest) -> NpcChatStartResponse:
    await cleanup_chat_jobs()
    persona = find_persona(req)

    if hail_detected(req):
        response = hail_response(req, persona)
        return NpcChatStartResponse(ok=True, done=True, status="done", response=response.response, reason=response.reason)

    if ooc_input_detected(req):
        response = grounded_fallback_response(req, persona, "ooc_input", 0)
        return NpcChatStartResponse(ok=response.ok, done=True, status="done", response=response.response, reason=response.reason)

    if zone_advice_question(req):
        response = grounded_fallback_response(req, persona, "zone_advice", 0)
        return NpcChatStartResponse(ok=response.ok, done=True, status="done", response=response.response, reason=response.reason)

    if not deep_lookup_question(req):
        response = await npc_chat(req)
        return NpcChatStartResponse(ok=response.ok, done=True, status="done", response=response.response, reason=response.reason)

    job_id = uuid.uuid4().hex
    async with CHAT_JOBS_LOCK:
        CHAT_JOBS[job_id] = {
            "status": "pending",
            "created_at": time.time(),
            "updated_at": time.time(),
            "request": req.model_dump(),
            "result": None,
        }

    asyncio.create_task(process_chat_job(job_id))
    ack = persona.get("deep_ack", "Hmm. Give me a moment to recall the old records.")
    return NpcChatStartResponse(
        ok=True,
        done=False,
        status="pending",
        job_id=job_id,
        ack_response=clean_npc_speech(str(ack)),
        poll_after_ms=CHAT_JOB_POLL_AFTER_MS,
    )


@app.get("/eqemu/npc-chat/result/{job_id}", response_model=NpcChatResultResponse)
async def npc_chat_result(job_id: str) -> NpcChatResultResponse:
    await cleanup_chat_jobs()
    async with CHAT_JOBS_LOCK:
        job = CHAT_JOBS.get(job_id)

    if not job:
        return NpcChatResultResponse(
            ok=False,
            done=True,
            status="expired",
            job_id=job_id,
            response="The thread slips away from me. Ask again, and I will try to catch it.",
            fallback=True,
            reason="job_not_found",
        )

    status = str(job.get("status", "pending"))
    if status != "done":
        return NpcChatResultResponse(ok=True, done=False, status=status, job_id=job_id)

    result = job.get("result") or {}
    return NpcChatResultResponse(
        ok=bool(result.get("ok", False)),
        done=True,
        status="done",
        job_id=job_id,
        response=str(result.get("response", "")),
        model=str(result.get("model", OLLAMA_DEEP_MODEL)),
        latency_ms=int(result.get("latency_ms", 0)),
        fallback=bool(result.get("fallback", False)),
        reason=result.get("reason"),
        sources=list(result.get("sources", [])),
    )


@app.post("/eqemu/npc-chat", response_model=NpcChatResponse)
async def npc_chat(req: NpcChatRequest) -> NpcChatResponse:
    started = time.perf_counter()
    persona = find_persona(req)
    if hail_detected(req):
        return hail_response(req, persona)
    if ooc_input_detected(req):
        return grounded_fallback_response(req, persona, "ooc_input", 0)
    if zone_advice_question(req):
        return grounded_fallback_response(req, persona, "zone_advice", 0)
    if unknown_lore_question(req):
        return grounded_fallback_response(req, persona, "unknown_lore", 0)

    try:
        raw = await call_ollama(req, persona)
        latency_ms = int((time.perf_counter() - started) * 1000)
        cleaned = clean_npc_speech(raw)
        if not cleaned:
            return fallback_response(req, persona, "empty_model_response", latency_ms)
        failure_reason = grounded_output_failure(req, cleaned)
        if failure_reason:
            return grounded_fallback_response(req, persona, failure_reason, latency_ms)
        return NpcChatResponse(
            ok=True,
            response=cleaned,
            model=OLLAMA_MODEL,
            latency_ms=latency_ms,
            fallback=False,
        )
    except httpx.TimeoutException:
        latency_ms = int((time.perf_counter() - started) * 1000)
        return grounded_fallback_response(req, persona, "ollama_timeout", latency_ms)
    except httpx.HTTPStatusError as exc:
        latency_ms = int((time.perf_counter() - started) * 1000)
        reason = f"ollama_http_{exc.response.status_code}"
        log.warning("Ollama HTTP error: %s", exc)
        return grounded_fallback_response(req, persona, reason, latency_ms)
    except Exception as exc:  # noqa: BLE001
        latency_ms = int((time.perf_counter() - started) * 1000)
        log.warning("NPC chat failed: %s", exc)
        return grounded_fallback_response(req, persona, "bridge_error", latency_ms)
