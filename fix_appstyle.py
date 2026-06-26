"""Remove orphaned raw string content from AppStyle.cpp (lines 360-1902)."""

with open('src/ui/AppStyle.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Keep lines 1-359 (0-indexed: 0-358), then lines 1903-1905 (0-indexed: 1902-1904)
# Line 359 ends with: kSidebarTextSelected));
# Line 1903 is: }
# Line 1904 is: (empty)
# Line 1905 is: }

new_lines = lines[:359] + lines[1902:]

with open('src/ui/AppStyle.cpp', 'w', encoding='utf-8') as f:
    f.writelines(new_lines)

print(f"Removed {1902-359} lines. New file has {len(new_lines)} lines.")
