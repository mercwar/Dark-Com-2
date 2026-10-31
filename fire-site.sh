#!/bin/bash
# IDENTITY: VERSION 3.9 // FIRE-SITE // CVBGOD
# ROLE: Dynamic Sitemap Generation with Robust XML Escaping and Target Protection.

# 1. Extract and sanitize target remote tracking nodes
REPO_URL=$(git config --get remote.origin.url | sed 's/\.git$//' | sed 's/git@github.com:/https:\/\/github.com\//')
BRANCH="main"
BASE_URL="${REPO_URL}/blob/${BRANCH}"

OUTPUT_FILE="sitemap.xml"

echo "Generating protected sitemap for: ${BASE_URL}"

# 2. Initialize conformant XML structural headers
echo '<?xml version="1.0" encoding="UTF-8"?>' > "$OUTPUT_FILE"
echo '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">' >> "$OUTPUT_FILE"

# 3. Traversal processing engine with strict asset exclusion filters
find . -type f -not -path '*/.git/*' -not -name "$OUTPUT_FILE" | while read -r file; do
    # Strip runtime node path tokens
    CLEAN_PATH="${file#./}"
    
    # Absolute XML entity compliance validation escaping
    SAFE_PATH=$(echo "$CLEAN_PATH" | sed 's/&/\&amp;/g' | sed 's/</\&lt;/g' | sed 's/>/\&gt;/g' | sed "s/'/\&apos;/g" | sed 's/"/\&quot;/g')
    
    # URL Percent-Encoding implementation envelope
    ENCODED_PATH=$(echo "$SAFE_PATH" | sed 's/ /%20/g')
    
    # Structuring node entries safely
    echo "  <url>" >> "$OUTPUT_FILE"
    echo "    <loc>${BASE_URL}/${ENCODED_PATH}</loc>" >> "$OUTPUT_FILE"
    echo "  </url>" >> "$OUTPUT_FILE"
done

# 4. Finalize tree nodes
echo '</urlset>' >> "$OUTPUT_FILE"

echo "[SUCCESS] Normalized sitemap written out to: $OUTPUT_FILE"
