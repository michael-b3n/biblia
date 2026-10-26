-- Bundled script, offers the other scripts the files of the Digital Bible Library as scripts.dbl: the metadata file of its
-- bundles. https://library.bible/
-- Writing scripts: doc/lua_scripts.md of the VerseLens repository.

local dbl = {}

---
--- \return the text of the first element of the tree \p root matching one of the \p paths, e.g.
--- { "identification/name" }. The sections of a path are found in this order in the path of an element, the element
--- closest to the root wins.
---
local function text_at(root, paths)
  for _, sections in ipairs(paths) do
    local found, found_depth
    local function walk(node, path, depth)
      local position = 1
      for _, section in ipairs(sections) do
        local _, finish = string.find(path, section, position, true)
        if not finish then
          position = nil
          break
        end
        position = finish + 1
      end
      if position and (not found_depth or depth < found_depth) then
        found, found_depth = node, depth
      end
      for _, child in ipairs(node.children) do
        if type(child) == "table" then
          walk(child, path .. "/" .. child.name, depth + 1)
        end
      end
    end
    walk(root, "/" .. root.name, 0)
    if found then
      return util.xml_text(found, true)
    end
  end
  return nil
end

---
--- \return the `name`, `abbreviation`, `language` and `copyright` of the metadata file `metadata.xml` \p text of a
--- bundle, the local ones first, each nil if the file has none, or nil and the reason if it is no XML
---
function dbl.info(text)
  local tree, reason = util.parse_xml(text)
  if not tree then
    return nil, reason
  end
  return {
    name = text_at(tree, { { "identification/nameLocal" }, { "identification/name" }, { "name" } }),
    abbreviation = text_at(tree, {
      { "identification/abbreviationLocal" }, { "identification/abbreviation" }, { "abbreviation" },
    }),
    language = text_at(tree, { { "language/nameLocal" }, { "language/name" }, { "language" } }),
    copyright = text_at(tree, { { "copyright", "statementContent" }, { "copyright" } }),
  }
end

-- Shown to the user, other scripts see the functions above at scripts.dbl
dbl.name = "DBL"
dbl.description = "Reads the metadata of the bundles of the Digital Bible Library."
dbl.node = "dbl"
return dbl
