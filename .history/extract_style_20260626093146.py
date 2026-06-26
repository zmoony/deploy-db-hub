"""Extract the raw style sheet into a const char* array for compilation."""
import re

with open('src/ui/AppStyle.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find the raw string content between R"STYLE( and )STYLE"
start = content.find('R"STYLE(') + len('R"STYLE(')
end = content.find(')STYLE"')
raw_content = content[start:end]

# Split into chunks of ~100 lines each
lines = raw_content.split('\n')
chunks = []
chunk_size = 100
for i in range(0, len(lines), chunk_size):
    chunk = '\n'.join(lines[i:i+chunk_size])
    # Escape backslashes and quotes for C++ string literal
    chunk = chunk.replace('\\', '\\\\')
    chunk = chunk.replace('"', '\\"')
    chunks.append(chunk)

# Generate the C++ code
print(f"// Generated {len(chunks)} chunks from {len(lines)} lines")
for i, chunk in enumerate(chunks):
    print(f'    "{chunk}\\n"')
