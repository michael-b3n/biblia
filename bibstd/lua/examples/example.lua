-- Example script, shows how a script offers a scripture. Inactive, set `enabled` to true to try it.
-- Writing scripts: doc/lua_scripts.md of the VerseLens repository.

local enabled = false
if not enabled then
  return
end

-- The cache of this script, a name of letters, digits, "_" and "-"
local cache = "example"

return {
  -- Shown to the user
  name = "Example",
  functions = {
    ---
    --- \return the names of the scriptures this script offers
    ---
    ["scripture.names"] = function()
      return { names = { "DEMO" } }
    end,

    ---
    --- \return abbreviation, language and copyright of the scripture \p input.name
    ---
    ["scripture.information"] = function(input)
      return { abbreviation = input.name, language = "en", copyright = "Example script" }
    end,

    ---
    --- \return the names of the book \p input.book, nil if the script has none
    ---
    ["scripture.book"] = function(input)
      if input.book == "john" then
        return { short_name = "John" }
      end
    end,

    ---
    --- \return the text of the verse of \p input, kept in the cache once made
    ---
    ["scripture.passage"] = function(input)
      local key = table.concat({ input.name, input.book, input.chapter, input.verse }, ".")
      local cached = workflow.cache.get(cache, key)
      if cached then
        return { text = cached }
      end
      -- A script providing real text fetches it here, mind the terms of use of the page:
      -- local page, reason = workflow.web.fetch("https://example.com/" .. input.book .. input.chapter)
      -- if not page then return { error = "example.com: " .. reason } end
      local text = "Example text of " .. input.book .. " " .. input.chapter .. "," .. input.verse
      workflow.cache.set(cache, key, text)
      return { text = text }
    end,
  },
}
