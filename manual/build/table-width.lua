-- table-width.lua
-- For the LaTeX/PDF build, the widths of the columns of a table rounded
-- down to two digits: the LaTeX writer prints them with two digits, rounded
-- to the nearest, so that the columns of a table could add up to a little
-- more than the width of the text and overflow it.

function Table(el)
  for i, cs in ipairs(el.colspecs) do
    if type(cs[2]) == 'number' then
      el.colspecs[i] = { cs[1], math.floor(cs[2] * 100 + 1e-9) / 100 }
    end
  end
  return el
end
