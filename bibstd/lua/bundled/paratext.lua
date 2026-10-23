-- Bundled script, offers the other scripts the files of Paratext, the editor of Bible translations, as scripts.paratext:
-- the settings file of a project.
-- https://paratext.org/
-- Writing scripts: doc/lua_scripts.md of the VerseLens repository.

local paratext = {}

---
--- \return the `name`, `abbreviation`, `language` and `copyright` of the settings file `Settings.xml` \p text of a
--- project, each nil if the file has none, or nil and the reason if it is no XML
---
function paratext.info(text)
  local tree, reason = util.parse_xml(text)
  if not tree then
    return nil, reason
  end
  local function text_of(name)
    local child = util.filter(tree.children, function(c) return type(c) == "table" and c.name == name end)[1]
    return child and util.xml_text(child, true) or nil
  end
  return {
    name = text_of("FullName"),
    abbreviation = text_of("Name"),
    language = text_of("Language"),
    copyright = text_of("Copyright"),
  }
end

-- Shown to the user, other scripts see the functions above at scripts.paratext
paratext.name = "Paratext"
paratext.node = "paratext"
return paratext
