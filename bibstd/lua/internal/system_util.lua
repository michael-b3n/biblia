-- Helpers of the app, out of reach of scripts.
local util = {}

---
--- Remove the last of \p sections from \p index on below \p node, and the tables on its way it leaves empty.
---
local function remove(node, sections, index)
  local section = sections[index]
  if index == #sections then
    node[section] = nil
    return
  end
  local child = node[section]
  if type(child) ~= "table" then
    return
  end
  remove(child, sections, index + 1)
  if next(child) == nil then
    node[section] = nil
  end
end

---
--- Remove what is at \p path of sections separated by "." in what scripts see, e.g. a function taken back, and the
--- tables on its way it leaves empty.
---
function util.remove(path)
  local sections = {}
  for section in string.gmatch(path, "[^.]+") do
    sections[#sections + 1] = section
  end
  if #sections > 0 then
    remove(root.interface, sections, 1)
  end
end

root.system.util = util
