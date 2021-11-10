" JDH-8 ASSEMBLER SYNTAX FILE

if exists("b:current_syntax")
  finish
endif

syn keyword jdhSpecial \$
syn keyword jdhReg a b c d f h l z
syn keyword jdhTodo contained TODO
syn match jdhComment ";.*$" contains=jdhTodo
syn match jdhDirective "^\s*[@][a-zA-Z]\+"
syn match jdhMacroArg "%[a-zA-Z0-9_]\+"
syn match jdhNumber "0x[0-9a-fA-F]\+"
syn match jdhNumber "\<[0-9]\+D\=\>"
syn match jdhOp "^\s*[a-zA-Z0-9_]\+\s"
syn match jdhOp "^\s*[a-zA-Z0-9_]\+$"
syn match jdhMicroOp "^\s*[~]\=[a-zA-Z0-9_]\+[,]\s*\([~]\=[a-zA-Z0-9_]\+[,]\=\s*\)\+"

" syn region jdhOp start='^' end='$'
syn region jdhLabel start="^\s*[a-zA-Z0-9_.]" end=":" oneline contains=jdhLabelName,jdhMacroArg,jdhAddr
syn region jdhString start='"' end='"'
syn region jdhAddr start='\[' end='\]' contains=jdhMacroArg,jdhAddrLabel

syn match jdhLabelName "^[a-zA-Z0-9_\.]\+:\=" contained
syn match jdhAddrLabel '[a-zA-Z0-9_\.]\+' contained

let b:current_syntax = "jdh8"
hi def link jdhSpecial Special
hi def link jdhTodo Todo
hi def link jdhComment Comment
hi def link jdhLabelName Label
hi def link jdhAddrLabel Label
hi def link jdhDirective Macro
hi def link jdhOp Function
hi def link jdhMicroOp Function
hi def link jdhMacroArg Special
hi def link jdhReg Identifier
hi def link jdhNumber Number
hi def link jdhString String
