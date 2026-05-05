#!/usr/bin/env bash
# Update gettext .po/.pot files for Sphinx documentation translations.
# Run this after editing English .rst sources to sync translation templates.
#
# Usage:
#   ./update-translation.sh              # update all languages
#   ./update-translation.sh zh_CN        # update a specific language
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
POT_DIR="$BUILD_DIR/gettext"
LOCALE_DIR="$SCRIPT_DIR/locale"

# Files intentionally left untranslated (comma-separated basenames without .rst)
SKIP_TRANSLATION="changelog"

# ---------------------------------------------------------------------------
# Step 1: Generate .pot templates from English .rst sources
# ---------------------------------------------------------------------------
echo "=== Generating POT files ==="
mkdir -p "$POT_DIR"

sphinx-build -b gettext \
    -D "language=en" \
    "$SCRIPT_DIR" \
    "$POT_DIR"

# ---------------------------------------------------------------------------
# Step 2: Update .po files for the requested language(s)
# ---------------------------------------------------------------------------
update_lang() {
    local lang="$1"
    local po_dir="$LOCALE_DIR/$lang/LC_MESSAGES"

    if [ ! -d "$po_dir" ]; then
        echo "  No translation directory for '$lang', skipping."
        return
    fi

    echo "=== Updating $lang .po files ==="

    # Helper: update a .po file from its .pot template
    update_po() {
        local po_path="$1"
        local pot_path="$2"
        local name="$(basename "$po_path" .po)"

        # Check skip list
        for skip in ${SKIP_TRANSLATION//,/ }; do
            if [ "$name" = "$skip" ]; then
                echo "  Skipping $name (intentionally untranslated)"
                return
            fi
        done

        if [ ! -f "$pot_path" ]; then
            echo "  WARNING: No POT for $name, skipping."
            return
        fi

        if msgmerge --update --backup=off "$po_path" "$pot_path"; then
            echo "  Updated: $name.po"
        else
            echo "  ERROR: msgmerge failed for $name.po"
            return 1
        fi
    }

    # Root-level .po files
    for pot in "$POT_DIR"/*.pot; do
        [ -f "$pot" ] || continue
        po="$po_dir/$(basename "$pot" .pot).po"
        [ -f "$po" ] || continue
        update_po "$po" "$pot"
    done

    # Subdirectory .po files
    for subdir in "$POT_DIR"/*/; do
        [ -d "$subdir" ] || continue
        local subname="$(basename "$subdir")"
        for pot in "$subdir"*.pot; do
            [ -f "$pot" ] || continue
            po="$po_dir/$subname/$(basename "$pot" .pot).po"
            [ -f "$po" ] || continue
            update_po "$po" "$pot"
        done
    done
}

if [ $# -gt 0 ]; then
    for lang in "$@"; do
        update_lang "$lang"
    done
else
    # Update all existing translations
    for lang_dir in "$LOCALE_DIR"/*/; do
        [ -d "$lang_dir" ] || continue
        lang="$(basename "$lang_dir")"
        update_lang "$lang"
    done
fi

echo ""
echo "=== Done ==="
echo "Review fuzzy entries with:  msgattrib --fuzzy <file>.po"
echo "Build docs with:  ./make-zh-doc.sh  (or ./make-doc.sh for English)"
