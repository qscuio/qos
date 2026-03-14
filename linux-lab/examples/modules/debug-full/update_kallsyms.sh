#!/bin/bash

# List of files to update
FILES=(
    "ftrace.c"
    "generic_access_phys.c"
    "kernel-sections.c"
    "kprobe.c"
    "percpu_var.c"
    "section.c"
)

# Add lookup_symbol.h include and update kallsyms_lookup_name calls
for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        # Add include
        sed -i '1i#include "lookup_symbol.h"' "$file"
        
        # Replace kallsyms_lookup_name calls
        sed -i 's/kallsyms_lookup_name(\([^)]*\))/lookup_name(\1)/g' "$file"
    fi
done 