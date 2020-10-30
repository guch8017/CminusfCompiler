add_library(common STATIC
    syntax_tree.c
    ast.cpp
)

target_link_libraries(common)

