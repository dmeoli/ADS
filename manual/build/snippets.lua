-- snippets.lua
-- Single-source inclusion of the code and of the pseudocode of the headers.
-- A chapter includes a file of ../generated (written by snarf.py) with the
-- line of pymdownx.snippets that MkDocs expands natively:
--   --8<-- "generated/ArrayStack-push.pseudo.md"     (a paragraph alone)
-- or, for code, the same line as the only content of a fenced code block.
-- For the Pandoc (PDF) build this filter does the same expansion: a code
-- block gets the text of the file, a paragraph the blocks of the file read
-- as Markdown. Paths are relative to the manual root (the parent of
-- build/), as in the base_path of mkdocs.yml. A missing file is an error,
-- not a silent omission.

local root = '..'

local function slurp(path)
  local f = io.open(root .. '/' .. path, 'r')
  if not f then
    error('snippets.lua: missing file ' .. path)
  end
  local text = f:read('a')
  f:close()
  return text
end

function CodeBlock(el)
  local path = el.text:match('^%-%-8<%-%-%s+"([^"]+)"%s*$')
  if path then
    el.text = slurp(path):gsub('%s+$', '')
    return el
  end
end

function Para(el)
  local c = el.content
  if #c == 3 and c[1].t == 'Str' and c[1].text == '\u{2013}8<\u{2013}'
     and c[2].t == 'Space' and c[3].t == 'Quoted' then
    local path = pandoc.utils.stringify(c[3].content)
    return pandoc.read(slurp(path), 'markdown+tex_math_dollars').blocks
  end
end
