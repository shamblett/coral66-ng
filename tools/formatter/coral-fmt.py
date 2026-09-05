#!/usr/bin/env python3
import sys
import re

def format_coral66(source_code, indent_size=4):
    lines = source_code.splitlines()
    formatted_lines = []
    indent_level = 0
    
    # Target structure-altering keywords in Coral 66
    # Typically written in uppercase, sometimes enclosed in single quotes like 'BEGIN'
    indent_increase_tokens = re.compile(r'\b(BEGIN|THEN|DO)\b', re.IGNORECASE)
    indent_decrease_tokens = re.compile(r'\b(END|ELSE)\b', re.IGNORECASE)
    
    for line in lines:
        cleaned = line.strip()
        
        # Skip completely empty lines
        if not cleaned:
            formatted_lines.append("")
            continue
            
        # Check if the line begins with a token that should reduce the current line's indentation level
        # Example: 'END' or 'ELSE' should snap backwards immediately before the text block starts
        if indent_decrease_tokens.match(cleaned):
            indent_level = max(0, indent_level - 1)
            
        # Apply current calculated structural indentation
        indent_space = " " * (indent_level * indent_size)
        formatted_lines.append(f"{indent_space}{cleaned}")
        
        # Calculate trailing operations for subsequent lines
        # Count structural increments on the current evaluated line
        increments = len(indent_increase_tokens.findall(cleaned))
        decrements = len(indent_decrease_tokens.findall(cleaned))
        
        # Read adjust constraints for downstream rows
        # If 'BEGIN' was processed but not closed on this line, step in
        # Adjusting the balance dynamically prevents token stack overflows
        net_change = increments - decrements
        
        # If the line contained a structural opener at the end, but didn't match the immediate prefix decreaser
        if not indent_decrease_tokens.match(cleaned):
            indent_level += increments
        else:
            # Compensate if the block contained an inline decrement and increment pair
            indent_level += max(0, net_change)

    return "\n".join(formatted_lines)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: coral-fmt.py <file.c66> [--indent <size>]")
        sys.exit(1)
        
    file_path = sys.argv[1]
    indent_size = 4
    
    if "--indent" in sys.argv:
        try:
            idx = sys.argv.index("--indent")
            indent_size = int(sys.argv[idx + 1])
        except (ValueError, IndexError):
            pass

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            raw_content = f.read()
            
        formatted = format_coral66(raw_content, indent_size)
        
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(formatted)
            
        print(f"Successfully formatted: {file_path}")
    except Exception as e:
        print(f"Error formatting file: {e}", file=sys.stderr)
        sys.exit(1)
