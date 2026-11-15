-- source-links.lua
-- The name of a file or folder of the repository written as code, e.g.,
-- `include/ADS/tree/BTree.h`, `tree/BTree.h` or `BTree.h`, becomes a link
-- to it on GitHub, as hooks.py does on the web site. The names and their
-- links, chapter by chapter, are those that `python3 hooks.py --tsv` prints,
-- so that the two builds resolve a name in the same way; the chapter of a
-- code span is that of the last heading of level 1 before it. Run before
-- break-code.lua, which leaves the link alone.

local REPO = 'https://github.com/dmeoli/ADS' -- the repository
local BRANCH = 'main' -- the branch published
local here = PANDOC_SCRIPT_FILE:match('(.*/)') or './'

local chapter_of, links = {}, {}
local tsv = pandoc.pipe('python3', { here .. 'hooks.py', '--tsv' }, '')
for line in tsv:gmatch('[^\n]+') do
  local a, b, c = line:match('^([^\t]+)\t([^\t]+)\t([^\t]+)$')
  if a == 'H' then
    chapter_of[b] = c
  elseif a then
    if c:sub(1, #REPO) ~= REPO or not c:find('/' .. BRANCH .. '/', 1, true)
    then
      error('source-links.lua: REPO or BRANCH differs from hooks.py')
    end
    links[a] = links[a] or {}
    links[a][b] = c
  end
end

local chapter = 'index'

local function code(el)
  local url = links[chapter] and links[chapter][el.text]
  if not url then
    return nil
  end
  return pandoc.Link({ el }, url)
end

local function walk(blocks)
  for i, b in ipairs(blocks) do
    if b.t == 'Header' and b.level == 1 and chapter_of[b.identifier] then
      chapter = chapter_of[b.identifier]
    elseif b.t ~= 'CodeBlock' then
      -- top-down, so that a code span already within a link stays as it is
      blocks[i] = pandoc.walk_block(b, {
        traverse = 'topdown',
        Link = function(el) return el, false end,
        Code = code,
      })
    end
  end
  return blocks
end

return {
  { Pandoc = function(doc) doc.blocks = walk(doc.blocks); return doc end }
}
