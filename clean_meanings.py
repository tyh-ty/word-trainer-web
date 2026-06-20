#!/usr/bin/env python3
"""Clean up duplicate meanings in cet-vocab.js.

The vocab data has meanings like:
  'n. 甲板；舱面；层面；n. 甲板，舱面；层面'
where the same definition appears twice from different dictionary sources.
This script deduplicates by splitting on POS-tag boundaries, normalizing,
and keeping only unique fragments.
"""
import json, re, sys

def dedup_meaning(meaning: str) -> str:
    if not meaning or len(meaning) < 10:
        return meaning

    # Split into POS-prefixed segments
    # e.g. "v. 丢弃；放弃，抛弃；vt. 离弃，丢弃；遗弃，抛弃；放弃；n. 放任"
    # -> ["v. 丢弃；放弃，抛弃", "vt. 离弃，丢弃；遗弃，抛弃；放弃", "n. 放任"]
    pos_tags = r'(?:n|v|adj|adv|vt|vi|prep|conj|pron|int|art|num|a)\.'
    segments = re.split(r'(?:；|;|，|,)\s*(?=' + pos_tags + r')', meaning)
    segments = [s.strip().rstrip('；;，, ') for s in segments if s.strip()]

    if len(segments) <= 1:
        return meaning

    # For each segment, extract: POS tag, and set of individual Chinese meanings
    def parse_segment(seg):
        m = re.match(r'(' + pos_tags + r')\s*(.*)', seg)
        if not m:
            return (seg, set())
        pos = m.group(1)
        rest = m.group(2)
        # Split on Chinese punctuation separators
        parts = re.split(r'[；;，,、]+', rest)
        parts = {p.strip() for p in parts if p.strip()}
        return (pos, parts)

    # Group by POS tag and merge meanings
    pos_groups = {}
    pos_order = []
    for seg in segments:
        pos, parts = parse_segment(seg)
        if pos not in pos_groups:
            pos_groups[pos] = set()
            pos_order.append(pos)
        pos_groups[pos].update(parts)

    # Rebuild
    result_parts = []
    for pos in pos_order:
        parts = pos_groups[pos]
        if not parts:
            continue
        # Remove empty strings
        parts = sorted(p for p in parts if p)
        if re.match(pos_tags, pos):
            result_parts.append(f"{pos} {'，'.join(parts)}")
        else:
            result_parts.append(pos)

    return '；'.join(result_parts)


def main():
    path = 'web/data/cet-vocab.js'
    with open(path, 'r', encoding='utf-8') as f:
        text = f.read()

    # The file structure:
    # window.CET_VOCAB_META = { ... };
    # window.CET_WORDS = [{ ... }, ...];
    m = re.search(r'(window\.CET_WORDS\s*=\s*)(\[.*\])(\s*;)', text, re.DOTALL)
    if not m:
        print("Could not find CET_WORDS array", file=sys.stderr)
        sys.exit(1)

    prefix_text = text[:m.start()]
    var_prefix = m.group(1)
    data = json.loads(m.group(2))
    suffix = m.group(3)
    remaining_text = text[m.end():]

    cleaned = 0
    for word in data:
        original = word.get('meaning', '')
        deduped = dedup_meaning(original)
        if deduped != original:
            word['meaning'] = deduped
            cleaned += 1

    # Write back
    json_str = json.dumps(data, ensure_ascii=False, indent=None, separators=(',', ':'))
    with open(path, 'w', encoding='utf-8') as f:
        f.write(prefix_text)
        f.write(var_prefix)
        f.write(json_str)
        f.write(suffix)
        f.write(remaining_text)

    print(f"Cleaned {cleaned} / {len(data)} meanings")

    # Verify some problematic ones
    for sp in ['deck', 'due', 'abandon', 'absent', 'abuse', 'abstract']:
        for w in data:
            if w['spelling'] == sp:
                print(f"  {sp}: {w['meaning']}")
                break


if __name__ == '__main__':
    main()
