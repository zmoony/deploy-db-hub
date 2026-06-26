with open('src/ui/AppStyle.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Find all QStringLiteral(R"(" occurrences
for i, line in enumerate(lines, start=1):
    if 'QStringLiteral(R"(' in line:
        print(f"Line {i}: QStringLiteral(R\"(\" found")
    if ')"' in line:
        print(f"  Line {i}: )\" found: {line.strip()[:80]}")
