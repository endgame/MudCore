-- This program is free software. It comes without any warranty, to
-- the extent permitted by applicable law. You can redistribute it
-- and/or modify it under the terms of the Do What The Fuck You Want
-- To Public License, Version 2, as published by Sam Hocevar. See
-- http://sam.zoy.org/wtfpl/COPYING for more details.

-- Map player names to their descriptor.
PLAYERS = {}

-- Remove leading/trailing whitespace.
function string:trim()
   return (self:gsub('^%s*(.-)%s*$', '%1'))
end

-- Read in a loop until a valid name has been read.
function mudcore.descriptor:read_name()
   local name = self:read():trim():lower()
   if name:match '^%l+$' then
      if PLAYERS[name] then
         self:send 'That name is already taken. Choose another.\r\n'
         return self:read_name()
      else
         return name
      end
   else
      self:send 'Invalid name. Choose another.\r\n'
      return self:read_name()
   end
end

function mudcore.descriptor:on_close()
   if self.name then
      PLAYERS[self.name] = nil
   end
end

-- The chat mainloop.
function mudcore.descriptor:chat()
   self.prompt = 'chat> '
   while true do
      local line = self:read()
      local message = '<' .. self.name .. '> ' .. line .. '\r\n'
      for _, d in pairs(PLAYERS) do
         if d ~= self then
            d:send(message)
         end
      end
   end
end

function mudcore.descriptor:on_open()
   self.prompt = '> '
   self:send 'By what name do you wish to be known?\r\n'
   self.name = self:read_name()
   PLAYERS[self.name] = self
   self:chat()
end
