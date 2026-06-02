#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
WEB_DIR="$PROJECT_DIR/web-flash"

# Source ESP-IDF
source /home/tenbox/esp/esp-idf/export.sh

cd "$SCRIPT_DIR"

# Build firmware
echo "=== Building firmware ==="
idf.py build

# Merge into single binary for web-flash
echo "=== Merging firmware ==="
mkdir -p "$WEB_DIR"
esptool.py --chip esp32s3 merge_bin \
    -o "$WEB_DIR/sticks3-toolkit-merged.bin" \
    --flash_mode dio --flash_size 8MB --flash_freq 80m \
    0x0 build/bootloader/bootloader.bin \
    0x8000 build/partition_table/partition-table.bin \
    0x10000 build/sticks3-toolkit.bin

# Also copy standalone app binary
cp build/sticks3-toolkit.bin "$WEB_DIR/sticks3-toolkit.bin"

# Compute SHA256 and write meta
SHA256=$(sha256sum "$WEB_DIR/sticks3-toolkit-merged.bin" | awk '{print $1}')
SIZE=$(stat -c%s "$WEB_DIR/sticks3-toolkit-merged.bin")

cat > "$WEB_DIR/fw-meta.json" << JSONEOF
{
  "version": "$(date +%Y%m%d-%H%M)",
  "sha256": "$SHA256",
  "size": $SIZE,
  "url": "sticks3-toolkit-merged.bin"
}
JSONEOF

echo ""
echo "=== Done ==="
echo "Merged:  $WEB_DIR/sticks3-toolkit-merged.bin ($(du -h "$WEB_DIR/sticks3-toolkit-merged.bin" | cut -f1))"
echo "SHA256:  $SHA256"
echo ""
