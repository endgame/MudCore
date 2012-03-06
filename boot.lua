-- This program is free software. It comes without any warranty, to
-- the extent permitted by applicable law. You can redistribute it
-- and/or modify it under the terms of the Do What The Fuck You Want
-- To Public License, Version 2, as published by Sam Hocevar. See
-- http://sam.zoy.org/wtfpl/COPYING for more details.

-- Map player names to their descriptor.
PLAYERS = {}

-- Remove leading/trailing whitespace.
function string.trim(s)
   return (s:gsub('^%s*(.-)%s*$', '%1'))
end

-- Read in a loop until a valid name has been read.
function mud.descriptor.read_name(desc)
   local name = mud.descriptor.read():trim():lower()
   if name:match '^%l+$' then
      if PLAYERS[name] then
         desc:send 'That name is already taken. Choose another.\r\n'
         return mud.descriptor.read_name(desc)
      else
         return name
      end
   else
      desc:send 'Invalid name. Choose another.\r\n'
      return mud.descriptor.read_name(desc)
   end
end

function mud.descriptor.on_close(desc)
   PLAYERS[desc.name] = nil
end

-- The chat mainloop.
function chat(desc)
   desc.prompt = 'chat> '
   while true do
      local line = mud.descriptor.read()
      local message = '<' .. desc.name .. '> ' .. line .. '\r\n'
      for _, d in pairs(PLAYERS) do
         if d ~= desc then
            d:send(message)
         end
      end
   end
end

function mud.descriptor.on_open(desc)
   desc.prompt = '> '
   desc:send 'By what name do you wish to be known?\r\n'
   desc.name = mud.descriptor.read_name(desc)
   PLAYERS[desc.name] = desc
   chat(desc)
end
