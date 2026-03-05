/*
experimental css color extractor -- can be pasted into the Chrome developer window
 */
(() => {
    // --- Conversion utilities ---

    // note: srgb conversion not performed (and is_linear flag set to false).
    // the expectation is that css colors have an srgb curve built into them
    // and it is preserved when pasting this in.
    //
    // However, this conversion could be added back in and is_linear: true
    // if someone wanted to do the transform at this stage.
    function srgbToLinear(c) {
        return c <= 0.04045 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4);
    }

    function floatToHex32(f) {
        const buf = new ArrayBuffer(4);
        new Float32Array(buf)[0] = f;
        return Array.from(new Uint8Array(buf))
            .reverse()
            .map(b => b.toString(16).padStart(2, '0'))
            .join('');
    }

    function parseColor(str) {
        const el = document.createElement('div');
        el.style.color = str;
        document.body.appendChild(el);
        const computed = getComputedStyle(el).color;
        document.body.removeChild(el);
        const m = computed.match(/rgba?\(\s*([\d.]+),\s*([\d.]+),\s*([\d.]+)(?:,\s*([\d.]+))?\)/);
        if (!m) return null;
        return {
            r: parseFloat(m[1]) / 255,
            g: parseFloat(m[2]) / 255,
            b: parseFloat(m[3]) / 255,
            a: m[4] !== undefined ? parseFloat(m[4]) : 1.0
        };
    }

    // --- Clean up a raw name ---
    function cleanName(raw) {
        return raw
            .replace(/\bcolor\b/gi, '')   // strip the word "color"
            .replace(/\s{2,}/g, ' ')       // collapse multiple spaces
            .trim();
    }

    // --- Extract colors with names from CSS ---
    const colorMap = new Map(); // key: "r,g,b,a" => name

    function addColor(value, name) {
        const c = parseColor(value);
        if (!c) return;
        const key = [c.r, c.g, c.b, c.a].map(v => v.toFixed(6)).join(',');
        if (!colorMap.has(key)) {
            colorMap.set(key, cleanName(name || value));
        }
    }

    // Walk all stylesheets for custom properties and named rules
    function extractFromStylesheets() {
        for (const sheet of document.styleSheets) {
            let rules;
            try { rules = sheet.cssRules || sheet.rules; } catch { continue; }
            if (!rules) continue;
            for (const rule of rules) {
                if (rule.style) {
                    for (let i = 0; i < rule.style.length; i++) {
                        const prop = rule.style[i];
                        const val = rule.style.getPropertyValue(prop).trim();
                        if (!val) continue;
                        // Custom properties (--foo-bar) make great names
                        if (prop.startsWith('--')) {
                            const name = prop.replace(/^--/, '').replace(/[-_]+/g, ' ').trim();
                            // May contain multiple color-like tokens; try the whole value first
                            const parsed = parseColor(val);
                            if (parsed) {
                                addColor(val, name);
                            }
                        } else if (/color|background|border|outline|shadow|fill|stroke/i.test(prop)) {
                            // Extract color tokens from value
                            const tokens = val.match(/#[0-9a-fA-F]{3,8}\b|rgba?\([^)]+\)|hsla?\([^)]+\)|\b[a-z]{3,20}\b/gi);
                            if (tokens) {
                                for (const tok of tokens) {
                                    // Skip non-color keywords
                                    if (/^(none|inherit|initial|unset|auto|transparent|currentcolor|solid|dashed|dotted|inset|normal)$/i.test(tok)) continue;
                                    const propName = prop.replace(/[-_]+/g, ' ').trim();
                                    const selectorHint = (rule.selectorText || '').replace(/[.#:\[\]>~+*]/g, ' ').trim().split(/\s+/).pop() || '';
                                    const name = selectorHint ? `${selectorHint} ${propName}` : propName;
                                    addColor(tok, name);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Also grab computed styles from visible elements
    function extractFromComputed() {
        const props = ['color', 'backgroundColor', 'borderColor', 'borderTopColor',
            'borderRightColor', 'borderBottomColor', 'borderLeftColor',
            'outlineColor', 'textDecorationColor', 'caretColor', 'fill', 'stroke'];
        const seen = new Set();
        const els = document.querySelectorAll('*');
        for (const el of els) {
            const cs = getComputedStyle(el);
            for (const p of props) {
                const val = cs[p];
                if (!val || val === 'rgba(0, 0, 0, 0)' || seen.has(val)) continue;
                seen.add(val);
                // Try to derive name from element context
                const tag = el.tagName.toLowerCase();
                const cls = (el.className && typeof el.className === 'string')
                    ? el.className.split(/\s+/)[0].replace(/[-_]+/g, ' ').trim() : '';
                const id = el.id ? el.id.replace(/[-_]+/g, ' ').trim() : '';
                const propName = p.replace(/([A-Z])/g, ' $1').toLowerCase().trim();
                const hint = id || cls || tag;
                addColor(val, `${hint} ${propName}`);
            }
        }
    }

    extractFromStylesheets();
    extractFromComputed();

    // --- Build color entries and deduplicate names ---
    const colors = [];
    for (const [key, name] of colorMap) {
        const [r, g, b, a] = key.split(',').map(Number);
        colors.push({
            name: name,
            red: floatToHex32(r),
            green: floatToHex32(g),
            blue: floatToHex32(b),
            alpha: floatToHex32(a)
        });
    }

    // Deduplicate names: if "accent" appears twice, second becomes "accent (1)", etc.
    const nameCounts = new Map();
    for (const c of colors) {
        const base = c.name;
        if (!nameCounts.has(base)) {
            nameCounts.set(base, 1);
        } else {
            let n = nameCounts.get(base);
            let candidate = `${base} (${n})`;
            // walk up until truly unique across all existing names
            while (colors.some(x => x.name === candidate)) {
                n++;
                candidate = `${base} (${n})`;
            }
            c.name = candidate;
            nameCounts.set(base, n + 1);
        }
    }

    const now = Math.floor(Date.now() / 1000);
    // Simple hash from color data
    let hash = 0;
    for (const c of colors) {
        for (const ch of (c.red + c.green + c.blue)) {
            hash = ((hash << 5) - hash + ch.charCodeAt(0)) | 0;
        }
    }

    const palette = {
        palettes: [{
            title: document.title || 'untitled',
            color_hash: String(Math.abs(hash)),
            source: {
                conversion_tool: 'ftg_palette css_extractor.js',
                conversion_date: String(now)
            },
            color_space: {
                is_linear: false
            },
            colors: colors,
            hints: {},
            gradients: {},
            dither_pairs: {}
        }]
    };

    const json = JSON.stringify(palette, null, 4);
    console.log(json);

    // Also offer download
    const blob = new Blob([json], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = (document.title || 'palette').replace(/[^a-z0-9]+/gi, '_') + '.json';
    a.click();
    URL.revokeObjectURL(url);

    console.log(`Extracted ${colors.length} unique colors.`);
})();
