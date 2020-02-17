autocmd BufNewFile,BufRead *.h set filetype=c

call plug#begin('~/.vim/plugged')

Plug 'justmao945/vim-clang'

let g:clang_format_auto=1
let g:clang_check_syntax_auto=1
let g:clang_format_style='file'
let g:clang_format_exec='./clang-format-wrapper'

call plug#end()
